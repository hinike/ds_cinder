#include "touch_process.h"
#include "ds/math/math_defs.h"
#include "ds/ui/sprite/sprite.h"
#include "multi_touch_constraints.h"

namespace {
  const float MIN_TOUCH_DISTANCE = 20.0f;
  const float MIN_TAP_DISTANCE = 20.0f;
  const int SWIPE_QUEUE_SIZE = 4;
  const float DOUBLE_TAP_TIME = 0.2f;
}

namespace ds {
namespace ui {

TouchProcess::TouchProcess( Sprite &sprite )
  : mSprite(sprite)
  , mTappable(false)
  , mOneTap(false)
{
  mFingers.clear();
}

TouchProcess::~TouchProcess()
{

}

bool TouchProcess::processTouchInfo( const TouchInfo &touchInfo )
{
  if (!mSprite.visible() || !mSprite.isEnabled())
    return false;

  processTap(touchInfo);

  if (TouchInfo::Added == touchInfo.mPhase) {
    mFingers[touchInfo.mFingerId] = touchInfo;

    if (mFingers.size() == 1) {
      mSwipeQueue.clear();
      mSwipeFingerId = touchInfo.mFingerId;
      addToSwipeQueue(touchInfo.mCurrentPoint, 0);
    }

    initializeTouchPoints();
    sendTouchInfo(touchInfo);
    updateDragDestination(touchInfo);

    if (!mSprite.multiTouchEnabled())
      return true;
  } else if (TouchInfo::Moved == touchInfo.mPhase) {
    if (mFingers.empty())
      return false;

    auto found = mFingers.find(touchInfo.mFingerId);
    if (found == mFingers.end())
      return false;
    found->second.mCurrentPoint = touchInfo.mCurrentPoint;

    if (mSwipeFingerId == touchInfo.mFingerId)
      addToSwipeQueue(touchInfo.mCurrentPoint, 0);


    auto foundControl0 = mFingers.find(mControlFingerIndexes[0]);
    auto foundControl1 = mFingers.find(mControlFingerIndexes[1]);
    if (foundControl0 == mFingers.end() || foundControl1 == mFingers.end()) {
      sendTouchInfo(touchInfo);
      updateDragDestination(touchInfo);
    }

    if (mSprite.multiTouchEnabled() && (touchInfo.mFingerId == foundControl0->second.mFingerId || touchInfo.mFingerId == foundControl1->second.mFingerId)) {
      Matrix44f parentTransform;
      parentTransform.setToIdentity();

      Sprite *currentParent = mSprite.getParent();
      while (currentParent) {
        parentTransform = currentParent->getInverseTransform() * parentTransform;
      }

      Vec2f fingerStart0 = foundControl0->second.mStartPoint;
      Vec2f fingerCurrent0 = foundControl0->second.mCurrentPoint;
      Vec2f fingerPositionOffset = (parentTransform * Vec4f(fingerCurrent0.x, fingerCurrent0.y, 0.0f, 1.0f) - parentTransform * Vec4f(fingerStart0.x, fingerStart0.y, 0.0f, 1.0f)).xy();

      if (mFingers.size() > 1) {
        Vec2f fingerStart1 = foundControl1->second.mStartPoint;
        Vec2f fingerCurrent1 = foundControl1->second.mCurrentPoint;

        mStartDistance = fingerStart0.distance(fingerStart1);
        if (mStartDistance < MIN_TOUCH_DISTANCE)
          mStartDistance = MIN_TOUCH_DISTANCE;
        
        mCurrentDistance = fingerCurrent0.distance(fingerCurrent1);
        if (mCurrentDistance < MIN_TOUCH_DISTANCE)
          mCurrentDistance = MIN_TOUCH_DISTANCE;

        mCurrentDistance = mCurrentDistance / mStartDistance;

        mCurrentAngle = atan2(fingerStart1.y - fingerStart0.y, fingerStart1.x - fingerStart0.x) -
                        atan2(fingerCurrent1.y - fingerCurrent0.y, fingerCurrent1.x - fingerCurrent0.x);

        if (mSprite.hasMultiTouchConstraint(MULTITOUCH_CAN_SCALE)) {
          mSprite.setScale(mStartScale*mCurrentScale);
        }

        if (mSprite.hasMultiTouchConstraint(MULTITOUCH_CAN_ROTATE)) {
          mSprite.setRotation(mStartRotation - mCurrentAngle * math::RADIAN2DEGREE);
        }
      }

      if (mSprite.multiTouchConstraintNotZero() && touchInfo.mFingerId == foundControl0->second.mFingerId) {
        Vec2f offset(0.0f, 0.0f);
        if (!mTappable && mSprite.hasMultiTouchConstraint(MULTITOUCH_CAN_POSITION_X))
          offset.x = fingerPositionOffset.x;
        if (!mTappable && mSprite.hasMultiTouchConstraint(MULTITOUCH_CAN_POSITION_Y))
          offset.y = fingerPositionOffset.y;
        mSprite.setPosition(mStartPosition + offset);
      }
    }

    sendTouchInfo(touchInfo);
    updateDragDestination(touchInfo);
  } else if (TouchInfo::Removed == touchInfo.mPhase) {
    sendTouchInfo(touchInfo);
    updateDragDestination(touchInfo);

    auto found = mFingers.find(touchInfo.mFingerId);
    if (found != mFingers.end())
      mFingers.erase(found);

    if (!mSprite.multiTouchEnabled())
      return true;

    if (mFingers.empty())
      resetTouchAnchor();
    else
      initializeTouchPoints();

    if (mFingers.empty() && swipeHappened()) {
      mSprite.swipe(mSwipeVector);
    }
  }

  return true;
}

void TouchProcess::update( const UpdateParams &updateParams )
{
  if (!mSprite.visible() || !mSprite.isEnabled() || !mOneTap || !mSprite.hasDoubleTap())
    return;
  if (mLastUpdateTime - mDoubleTapTime > DOUBLE_TAP_TIME) {
    mSprite.tap(mFirstTapPos);
    mOneTap = false;
    mDoubleTapTime = mLastUpdateTime;
  }
}

void TouchProcess::sendTouchInfo( const TouchInfo &touchInfo )
{
  TouchInfo t = touchInfo;
  t.mCurrentAngle = mCurrentAngle;
  t.mCurrentScale = mCurrentScale;
  t.mCurrentDistance = mCurrentDistance;
  t.mStartDistance = mStartDistance;
  t.mNumberFingers = mFingers.size();

  if (touchInfo.mPhase == TouchInfo::Removed && t.mNumberFingers > 0)
    t.mNumberFingers = -1;

  auto found = mFingers.find(touchInfo.mFingerId);
  if (found != mFingers.end())
    t.mActive = found->second.mActive;

  mSprite.processTouchInfoCallback(touchInfo);
}

void TouchProcess::initializeFirstTouch()
{
  mFingers[mControlFingerIndexes[0]].mActive = true;
  mFingers[mControlFingerIndexes[0]].mStartPoint = mFingers[mControlFingerIndexes[0]].mCurrentPoint;
  mMultiTouchAnchor = mSprite.globalToLocal(mFingers[mControlFingerIndexes[0]].mStartPoint);
  mMultiTouchAnchor.x /= mSprite.getWidth();
  mMultiTouchAnchor.y /= mSprite.getHeight();
  mStartAnchor = mSprite.getCenter();

  Vec2f positionOffset = mMultiTouchAnchor - mStartAnchor;
  positionOffset.x *= mSprite.getWidth();
  positionOffset.y *= mSprite.getHeight();
  positionOffset.x *= mSprite.getScale().x;
  positionOffset.y *= mSprite.getScale().y;
  positionOffset.rotate(mSprite.getRotation() * math::DEGREE2RADIAN);
  if (mSprite.multiTouchConstraintNotZero()) {
    mSprite.setCenter(mMultiTouchAnchor);
    mSprite.move(positionOffset);
  }

  mStartPosition = mSprite.getPosition();
}

void TouchProcess::initializeTouchPoints()
{
  if (!mSprite.multiTouchEnabled())
    return;

  if (mFingers.size() == 1) {
    mControlFingerIndexes[0] = mFingers.begin()->first;
    resetTouchAnchor();
    initializeFirstTouch();
    return;
  }

  int potentialFarthestIndexes[2];
  float potentialFarthestDistance = 0.0f;

  for ( auto it = mFingers.begin(), it2 = mFingers.end(); it != it2; ++it )
  {
    it->second.mActive = false;
  	for ( auto itt = mFingers.begin(), itt2 = mFingers.end(); itt != itt2; ++itt )
  	{
  		if (it == itt)
        continue;
      float newDistance = itt->second.mCurrentPoint.distance(it->second.mCurrentPoint);
      if (newDistance > potentialFarthestDistance) {
        potentialFarthestIndexes[0] = itt->first;
        potentialFarthestIndexes[1] = it->first;
        potentialFarthestDistance = newDistance;
      }
  	}
  }

  if (fabs(potentialFarthestDistance) < math::EPSILON)
    return;

  mControlFingerIndexes[0] = potentialFarthestIndexes[0];
  mControlFingerIndexes[1] = potentialFarthestIndexes[1];

  initializeFirstTouch();

  mFingers[mControlFingerIndexes[1]].mActive = true;
  mFingers[mControlFingerIndexes[1]].mStartPoint = mFingers[mControlFingerIndexes[1]].mCurrentPoint;
  mStartPosition = mSprite.getPosition();
  mStartRotation = mSprite.getRotation();
  mStartScale    = mSprite.getScale();
  mStartWidth    = mSprite.getWidth();
  mStartHeight   = mSprite.getHeight();
}

void TouchProcess::resetTouchAnchor()
{
  if (!mSprite.multiTouchEnabled())
    return;
  Vec2f positionOffset = mStartAnchor - mSprite.getCenter();
  positionOffset.x *= mSprite.getWidth();
  positionOffset.y *= mSprite.getHeight();
  positionOffset.x *= mSprite.getScale().x;
  positionOffset.y *= mSprite.getScale().y;
  positionOffset.rotate(mSprite.getRotation() * math::DEGREE2RADIAN);
  if (mSprite.multiTouchConstraintNotZero()) {
    mSprite.setCenter(mMultiTouchAnchor);
    mSprite.move(positionOffset);
  }
}

void TouchProcess::addToSwipeQueue( const Vec2f &currentPoint, int queueNum )
{
  SwipeQueueEvent swipeEvent;
  swipeEvent.mCurrentPoint = currentPoint;
  swipeEvent.mTimeStamp = mLastUpdateTime;
  mSwipeQueue.push_back(swipeEvent);
  if (mSwipeQueue.size() > SWIPE_QUEUE_SIZE)
    mSwipeQueue.pop_front();
}

bool TouchProcess::swipeHappened()
{
  const float minSpeed = 800.0f;
  const float maxTimeThreshold = 0.5f;
  mSwipeVector = Vec2f();

  if (mSwipeQueue.size() < SWIPE_QUEUE_SIZE)
    return false;

  for ( auto it = mSwipeQueue.begin(), it2 = mSwipeQueue.end(); it != it2 - 1; ++it ) {
  	mSwipeVector += (it+1)->mCurrentPoint - it->mCurrentPoint;
  }

  mSwipeVector /= static_cast<float>(mSwipeQueue.size() - 1);
  float averageDistance = mSwipeVector.distance(Vec2f());

  return (averageDistance >= minSpeed * 60.0f && (mLastUpdateTime - mSwipeQueue.front().mTimeStamp) < maxTimeThreshold);
}

void TouchProcess::updateDragDestination( const TouchInfo &touchInfo )
{

}

void TouchProcess::processTap( const TouchInfo &touchInfo )
{
  if (!mSprite.hasTap() && !mSprite.hasDoubleTap()) {
    mTappable = false;
    return;
  }

  if (touchInfo.mPhase == TouchInfo::Added && mFingers.empty()) {
    mTappable = true;
  } else if (mTappable) {
    if (mFingers.size() > 1 || (touchInfo.mPhase == TouchInfo::Moved && touchInfo.mCurrentPoint.distance(touchInfo.mStartPoint) > MIN_TAP_DISTANCE)) {
      mTappable = false;
    } else if (touchInfo.mPhase == TouchInfo::Removed) {
      if (mSprite.hasTap() && !mSprite.hasDoubleTap()) {
        mSprite.tap(touchInfo.mCurrentPoint);
        mOneTap = false;
      } else if (mOneTap) {
        mSprite.doubleTap(touchInfo.mCurrentPoint);
        mOneTap = false;
      } else {
        mFirstTapPos = touchInfo.mCurrentPoint;
        mOneTap = true;
        mDoubleTapTime = mLastUpdateTime;
      }

      if (mFingers.size() == 1)
        mTappable = false;
    }
  }
}

}
}