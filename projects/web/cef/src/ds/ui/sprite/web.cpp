#include "web.h"

#include <algorithm>
#include <regex>
#include <cinder/ImageIo.h>
#include <boost/filesystem.hpp>
#include <ds/app/app.h>
#include <ds/app/engine/engine.h>
#include <ds/app/blob_reader.h>
#include <ds/app/environment.h>
#include <ds/data/data_buffer.h>
#include <ds/debug/logger.h>
#include <ds/math/math_func.h>
#include <ds/ui/sprite/sprite_engine.h>
#include <ds/ui/tween/tweenline.h>
#include <ds/util/string_util.h>
#include "private/web_service.h"



#include "include/cef_app.h"

#include "simple_app.h"
#include "simple_handler.h"

namespace {
// Statically initialize the world class. Done here because the Body is
// guaranteed to be referenced by the final application.
class Init {
public:
	Init() {
		ds::App::AddStartup([](ds::Engine& e) {
			ds::web::Service*		w = new ds::web::Service(e);
			if (!w) throw std::runtime_error("Can't create ds::web::Service");
			e.addService("cef_web", *w);

			e.installSprite([](ds::BlobRegistry& r){ds::ui::Web::installAsServer(r);},
							[](ds::BlobRegistry& r){ds::ui::Web::installAsClient(r);});
		});
	}
	void					doNothing() { }
};
Init						INIT;

char						BLOB_TYPE			= 0;
const ds::ui::DirtyState&	URL_DIRTY			= ds::ui::INTERNAL_A_DIRTY;
const ds::ui::DirtyState&	PDF_PAGEMODE_DIRTY	= ds::ui::INTERNAL_B_DIRTY;
const char					URL_ATT				= 80;
const char					PDF_PAGEMODE_ATT	= 81;
}

namespace ds {
namespace ui {

/**
 * \class ds::ui::sprite::Web static
 */
void Web::installAsServer(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromClient(r);});
}

void Web::installAsClient(ds::BlobRegistry& registry) {
	BLOB_TYPE = registry.add([](BlobReader& r) {Sprite::handleBlobFromServer<Web>(r);});
}

/**
 * \class ds::ui::sprite::Web
 */
Web::Web( ds::ui::SpriteEngine &engine, float width, float height )
	: Sprite(engine, width, height)
	, mService(engine.getService<ds::web::Service>("cef_web"))
	, mDragScrolling(false)
	, mDragScrollMinFingers(2)
	, mClickDown(false)
	, mPageScrollCount(0)
	, mDocumentReadyFn(nullptr)
	, mHasError(false)
	, mAllowClicks(true)
	, mBrowserId(-1)
	, mBuffer(nullptr)
	, mHasBuffer(false)
	, mBrowserSize(0, 0)
	, mUrl("")
{
	// Should be unnecessary, but really want to make sure that static gets initialized
	INIT.doNothing();

	mBlobType = BLOB_TYPE;
	mLayoutFixedAspect = true;

	setTransparent(false);
	setUseShaderTexture(true);
	setSize(width, height);

	enable(true);
	enableMultiTouch(ds::ui::MULTITOUCH_INFO_ONLY);

	setProcessTouchCallback([this](ds::ui::Sprite *, const ds::ui::TouchInfo &info) {
		handleTouch(info);
	});

	//const std::string urly = "downstream.com";
	//const std::string urly = "file://D:/content/sample_videos_2/vp9_4k.webm";
	//const std::string urly = "file://D:/test_pdfs/BPS C06_CIM_Services.pdf";
	//const std::string urly = "google.com";
	//const std::string urly = "http://i.imgur.com/r6sS64A.gifv";
	//const std::string urly = "https://google.com"; 
	//const std::string urly = "https://drive.google.com/drive/my-drive";
	//const std::string urly = "https://agoing.agsafoin.com/"; // generates an error cause the site can't be reached
	mService.createBrowser("", [this](int browserId){ 
		mBrowserId = browserId; 

		// Now that we know about the browser, set it to the correct size
		if(!mBuffer){
			onSizeChanged();
		} else {
			mService.requestBrowserResize(mBrowserId, mBrowserSize);
		}


		loadUrl(mUrl);

		mService.addPaintCallback(mBrowserId, [this](const void * buffer, const int bufferWidth, const int bufferHeight){
			// verify the buffer exists and is the correct size
			if(mBuffer && bufferWidth == mBrowserSize.x && bufferHeight == mBrowserSize.y){
				mHasBuffer = true;
				memcpy(mBuffer, buffer, bufferWidth * bufferHeight * 4);
			}
		});
	});

	
}

Web::~Web() {
	mService.addPaintCallback(mBrowserId, nullptr);

	if(mBuffer){
		delete mBuffer;
		mBuffer = nullptr;
	}
}

void Web::updateClient(const ds::UpdateParams &p) {
	Sprite::updateClient(p);

	update(p);
}

void Web::updateServer(const ds::UpdateParams &p) {
	Sprite::updateServer(p);

	mPageScrollCount = 0;

	update(p);
}

void Web::update(const ds::UpdateParams &p) {

	if(mBuffer && mHasBuffer){
		ci::gl::Texture::Format fmt;
		fmt.setMinFilter(GL_LINEAR);
		fmt.setMagFilter(GL_LINEAR);
		mWebTexture = ci::gl::Texture(mBuffer, GL_BGRA, mBrowserSize.x, mBrowserSize.y, fmt);
		mHasBuffer = false;
	}
}

void Web::onSizeChanged() {
	const int theWid = static_cast<int>(getWidth());
	const int theHid = static_cast<int>(getHeight());
	const ci::Vec2i newBrowserSize(theWid, theHid);
	if(newBrowserSize == mBrowserSize && mBuffer){
		return;
	}

	mBrowserSize = newBrowserSize;

	if(mBuffer){
		delete mBuffer;
		mBuffer = nullptr;
	}
	const int bufferSize = theWid * theHid * 4;
	mBuffer = new unsigned char[bufferSize];

	mHasBuffer = false;

	if(mBrowserId > -1){
		mService.requestBrowserResize(mBrowserId, mBrowserSize);
	}
}

void Web::drawLocalClient() {
	if (mWebTexture) {
		if(getPerspective()){
			ci::gl::draw(mWebTexture, ci::Rectf(0.0f, static_cast<float>(mWebTexture.getHeight()), static_cast<float>(mWebTexture.getWidth()), 0.0f));
		} else {
			ci::gl::draw(mWebTexture);
		}
	}
}

void Web::handleTouch(const ds::ui::TouchInfo& touchInfo) {
	if (touchInfo.mFingerIndex != 0)
		return;

	ci::Vec2f pos = globalToLocal(touchInfo.mCurrentGlobalPoint).xy();

	// this may not actually work in a rotated scenario, so...
	pos.x *= getScale().x;
	pos.y *= getScale().y;

	if (ds::ui::TouchInfo::Added == touchInfo.mPhase) {
		mService.sendMouseClick(mBrowserId, (int)roundf(pos.x), (int)(roundf(pos.y)), 0, 0, 1);

		/*
		ci::app::MouseEvent event(mEngine.getWindow(), ci::app::MouseEvent::LEFT_DOWN, static_cast<int>(pos.x), static_cast<int>(pos.y), ci::app::MouseEvent::LEFT_DOWN, 0, 1);
		sendMouseDownEvent(event);
		if(mDragScrolling){
			mClickDown = true;
		}
		*/
	} else if(ds::ui::TouchInfo::Moved == touchInfo.mPhase) {
		mService.sendMouseClick(mBrowserId, (int)roundf(pos.x), (int)(roundf(pos.y)), 0, 1, 1);


		if(mDragScrolling && touchInfo.mNumberFingers >= mDragScrollMinFingers){
			/*
			if(mWebViewPtr){
				if(mClickDown){
					ci::app::MouseEvent uevent(mEngine.getWindow(), ci::app::MouseEvent::LEFT_DOWN, static_cast<int>(pos.x), static_cast<int>(pos.y), 0, 0, 0);
					sendMouseUpEvent(uevent);
					mClickDown = false;
				}
				float yDelta = touchInfo.mCurrentGlobalPoint.y- mPreviousTouchPos.y;
				ci::app::MouseEvent event(mEngine.getWindow(), 0, static_cast<int>(pos.x), static_cast<int>(pos.y), ci::app::MouseEvent::LEFT_DOWN, yDelta, 1);
				ds::web::handleMouseWheel( mWebViewPtr, event, 1 );
			}
			*/
		} else {
		//	ci::app::MouseEvent event(mEngine.getWindow(), 0, static_cast<int>(pos.x), static_cast<int>(pos.y), ci::app::MouseEvent::LEFT_DOWN, 0, 1);
		//	sendMouseDragEvent(event);
		}
	} else if(ds::ui::TouchInfo::Removed == touchInfo.mPhase) {
		mService.sendMouseClick(mBrowserId, (int)roundf(pos.x), (int)(roundf(pos.y)), 0, 2, 1);
		//mClickDown = false;
		//ci::app::MouseEvent event(mEngine.getWindow(), ci::app::MouseEvent::LEFT_DOWN, static_cast<int>(pos.x), static_cast<int>(pos.y), 0, 0, 0);
		//sendMouseUpEvent(event);
	}

	mPreviousTouchPos = touchInfo.mCurrentGlobalPoint;
}


std::string Web::getUrl() {
	/*
	try {
	if (!mWebViewPtr) return "";
	Awesomium::WebURL		wurl = mWebViewPtr->url();
	Awesomium::WebString	webstr = wurl.spec();
	auto					len = webstr.ToUTF8(nullptr, 0);
	if (len < 1) return "";
	std::string				str(len+2, 0);
	webstr.ToUTF8(const_cast<char*>(str.c_str()), len);
	return str;
	} catch (std::exception const&) {
	}
	*/
	return "";
}

void Web::loadUrl(const std::wstring &url) {
	loadUrl(ds::utf8_from_wstr(url));
}

void Web::loadUrl(const std::string &url) {
	mUrl = url;
	if(mBrowserId > -1 && !mUrl.empty()){
		mService.loadUrl(mBrowserId, mUrl);
	}
}

void Web::setUrl(const std::string& url) {
	loadUrl(url);
}

void Web::setUrlOrThrow(const std::string& url) {
	loadUrl(url);
}

void Web::sendKeyDownEvent(const ci::app::KeyEvent &event) {
	/*
	if(mWebViewPtr){
		ds::web::handleKeyDown(mWebViewPtr, event);
	}
	*/
}

void Web::sendKeyUpEvent(const ci::app::KeyEvent &event){
	/*
	if(mWebViewPtr){
		ds::web::handleKeyUp(mWebViewPtr, event);
	}
	*/
}

void Web::handleNativeKeyEvent(const int state, int windows_key_code, int native_key_code, unsigned int modifiers, char character){
	mService.sendKeyEvent(mBrowserId, state, windows_key_code, native_key_code, modifiers, character);
}

void Web::sendMouseDownEvent(const ci::app::MouseEvent& e) {
	/*
	if (!mWebViewPtr || !mAllowClicks) return;

	ci::app::MouseEvent eventMove(mEngine.getWindow(), 0, e.getX(), e.getY(), 0, 0, 0);
	ds::web::handleMouseMove( mWebViewPtr, eventMove );
	ds::web::handleMouseDown( mWebViewPtr, e);
	sendTouchEvent(e.getX(), e.getY(), ds::web::TouchEvent::kAdded);
	*/
}

void Web::sendMouseDragEvent(const ci::app::MouseEvent& e) {
	/*
	if(!mWebViewPtr || !mAllowClicks) return;

	ds::web::handleMouseDrag(mWebViewPtr, e);
	sendTouchEvent(e.getX(), e.getY(), ds::web::TouchEvent::kMoved);
	*/
}

void Web::sendMouseUpEvent(const ci::app::MouseEvent& e) {
	/*
	if(!mWebViewPtr || !mAllowClicks) return;

	ds::web::handleMouseUp( mWebViewPtr, e);
	sendTouchEvent(e.getX(), e.getY(), ds::web::TouchEvent::kRemoved);
	*/
}

void Web::sendMouseClick(const ci::Vec3f& globalClickPoint){
	/*
	ci::Vec2f pos = globalToLocal(globalClickPoint).xy();

	ci::app::MouseEvent event(mEngine.getWindow(), ci::app::MouseEvent::LEFT_DOWN, static_cast<int>(pos.x), static_cast<int>(pos.y), ci::app::MouseEvent::LEFT_DOWN, 0, 1);
	sendMouseDownEvent(event);

	ci::app::MouseEvent eventD(mEngine.getWindow(), 0, static_cast<int>(pos.x), static_cast<int>(pos.y), ci::app::MouseEvent::LEFT_DOWN, 0, 1);
	sendMouseDragEvent(eventD);

	ci::app::MouseEvent eventU(mEngine.getWindow(), ci::app::MouseEvent::LEFT_DOWN, static_cast<int>(pos.x), static_cast<int>(pos.y), 0, 0, 0);
	sendMouseUpEvent(eventU);
	*/
}


void Web::setZoom(const double v) {
//	if (!mWebViewPtr) return;
//	mWebViewPtr->SetZoom(static_cast<int>(v * 100.0));
}

double Web::getZoom() const {
//	if (!mWebViewPtr) return 1.0;
//	return static_cast<double>(mWebViewPtr->GetZoom()) / 100.0;
	return 1.0;
}

void Web::goBack() {
	/*
	if (mWebViewPtr) {
		mWebViewPtr->Focus();
		mWebViewPtr->GoBack();
	}
	*/
}

void Web::goForward() {
	/*
	if (mWebViewPtr) {
		mWebViewPtr->GoForward();
	}
	*/
}

void Web::reload() {
	/*
	if (mWebViewPtr) {
		mWebViewPtr->Reload(true);
	}
	*/
}

void Web::stop() {
	/*
	if (mWebViewPtr) {
		mWebViewPtr->Stop();
	}
	*/
}

bool Web::canGoBack() {
	/*
	if (!mWebViewPtr) return false;
	return mWebViewPtr->CanGoBack();
	*/
	return true;
}

bool Web::canGoForward() {
	/*
	if (!mWebViewPtr) return false;
	return mWebViewPtr->CanGoForward();
	*/
	return true;
}

void Web::setAddressChangedFn(const std::function<void(const std::string& new_address)>& fn) {
//	if (mWebViewListener) mWebViewListener->setAddressChangedFn(fn);
}

void Web::setDocumentReadyFn(const std::function<void(void)>& fn) {
	mDocumentReadyFn = fn;
}

void Web::setErrorMessage(const std::string &message){
	mHasError = true;
	mErrorMessage = message;

	if(mErrorCallback){
		mErrorCallback(mErrorMessage);
	}
}

void Web::clearError(){
	mHasError = false;
}

ci::Vec2f Web::getDocumentSize() {
	// TODO
	return ci::Vec2f(getWidth(), getHeight());
}

ci::Vec2f Web::getDocumentScroll() {
	/*
	if (!mWebViewPtr) return ci::Vec2f(0.0f, 0.0f);
	return get_document_scroll(*mWebViewPtr);
	*/
	return ci::Vec2f::zero();
}

void Web::executeJavascript(const std::string& theScript){
	/*

	Awesomium::WebString		object_ws(Awesomium::WebString::CreateFromUTF8(theScript.c_str(), theScript.size()));
	Awesomium::JSValue			object = mWebViewPtr->ExecuteJavascriptWithResult(object_ws, Awesomium::WebString());
	std::cout << "Object return: " << ds::web::str_from_webstr(object.ToString()) << std::endl;
	*/
}

void Web::writeAttributesTo(ds::DataBuffer &buf) {
	ds::ui::Sprite::writeAttributesTo(buf);

	if (mDirty.has(URL_DIRTY)) {
		buf.add(URL_ATT);
		buf.add(mUrl);
	}
}

void Web::readAttributeFrom(const char attributeId, ds::DataBuffer &buf) {
	if (attributeId == URL_ATT) {
		setUrl(buf.read<std::string>());
	} else {
		ds::ui::Sprite::readAttributeFrom(attributeId, buf);
	}
}

bool Web::isLoading() {
	/*
	if(mWebViewPtr && mWebViewPtr->IsLoading()){
		return true;
	}
	*/
	return false;
}

bool Web::webViewDirty(){
	/*
	if(!mWebViewPtr){
		return false;
	}

	Awesomium::BitmapSurface* surface = (Awesomium::BitmapSurface*) mWebViewPtr->surface();
	if(!surface) return false; 

	return surface->is_dirty();
	*/
	return false;
}

void Web::onUrlSet(const std::string &url) {
	mUrl = url;
	markAsDirty(URL_DIRTY);
}

void Web::onDocumentReady() {
	/*
	if (!mWebViewPtr || !mJsMethodHandler) return;

	mJsMethodHandler->setDomIsReady(*mWebViewPtr);
	if (mDocumentReadyFn) mDocumentReadyFn();
	*/
}

void Web::setAllowClicks(const bool doAllowClicks){
	mAllowClicks = doAllowClicks;
}

void Web::setWebTransparent(const bool isTransparent){
	/*
	if(mWebViewPtr){
		mWebViewPtr->SetTransparent(isTransparent);
	}
	*/
}

} // namespace ui
} // namespace ds
