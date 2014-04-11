#pragma once
#ifndef DS_UI_TOUCH_INFO_H
#define DS_UI_TOUCH_INFO_H
#include "cinder/Vector.h"

namespace ds {
namespace ui {

class Sprite;

struct TouchInfo
{
	enum Phase
	{
		Added,
		Moved,
		Removed
	};

	int			mFingerIndex;
	int			mNumberFingers;
	int			mFingerId;
	Phase		mPhase;
	ci::Vec3f	mStartPoint;
	ci::Vec3f	mCurrentGlobalPoint;
	ci::Vec3f	mDeltaPoint;
	Sprite*		mPickedSprite;
	bool		mActive;
	float		mStartDistance;
	float		mCurrentDistance;
	float		mCurrentScale;
	float		mCurrentAngle;

	// This touch is being removed from it's previous owner if true
	bool		mPassedTouch;
};

} // namespace ui
} // namespace ds

#endif//DS_UI_TOUCH_INFO_H
