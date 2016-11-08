#pragma once
#ifndef DS_UI_SPRITE_WEB_H_
#define DS_UI_SPRITE_WEB_H_

#include <cinder/app/KeyEvent.h>
#include <cinder/app/MouseEvent.h>
#include "ds/ui/sprite/sprite.h"
#include "ds/ui/sprite/text.h"

#include <mutex>
#include <thread>

namespace ds {
namespace web {
	class WebCefService;
}

namespace ui {
/**
 * \class ds::ui::Web
 * \brief Display a web page using Chromium Embedded Framework: https://bitbucket.org/chromiumembedded/cef
 *		  The process and threading model here is complex.
 *		  In short, on creation, this sprite will asynchronously request an underlying browser from CefWebService.
 *        When the browser is created, this sprite will get a unique Browser Id that uses to make requests of the browser (back, load url, etc)
 *		  The browser also sends callbacks for certain events (loading state, fullscreen, etc)
 *		  The browser runs in it's own thread(s), so callbacks need to be locked before sending to the rest of ds_cinder-land
 *		  Callbacks are syncronized with the main thread using the mutex. They also need to happen outside the update loop, so they are cached and called via a 1-frame delay
 *		  Requests into the browser can (generally) happen on any thread, and CEF handles thread synchronization
 *		  CEF also uses multiple processes for rendering, IO, etc. but that is opaque to this class
 *		  When implementing new functionality, be sure to read the documentation of CEF carefully
 */
class Web : public ds::ui::Sprite {
public:
	Web(ds::ui::SpriteEngine &engine, float width = 0.0f, float height = 0.0f);
	~Web();


	// Loads the new url in the main frame (what you'd expect to happen)
	void										loadUrl(const std::wstring &url);
	void										loadUrl(const std::string &url);

	// setURL is identical to loadUrl. No exceptions will be thrown from any set or load url function.
	// load/set url are all kept around for compatibility with the old APIs
	void										setUrl(const std::string&);
	void										setUrlOrThrow(const std::string&);

	// returns the last URL set from loadUrl or setUrl (those two are identical). The page may have updated in the meantime
	std::string									getUrl();
	// returns the current url of the site as dispatched from CEF
	std::string									getCurrentUrl();

	// -------- Input Controls -------------------------------- //
	// If the sprite is being touched by mDragScrollMinFingers or more, will send mouse scroll events to the web view.
	void										setDragScrolling(const bool doScrolling){ mDragScrolling = doScrolling; }
	void										setDragScrollingMinimumFingers(const int numFingers){ mDragScrollMinFingers = numFingers; }

	void										sendKeyDownEvent(const ci::app::KeyEvent &event);
	void										sendKeyUpEvent(const ci::app::KeyEvent &event);

	// This web sprite handles touch-to-mouse events by default.
	// Though you can use these to roll your own touch stuff
	void										sendMouseDownEvent(const ci::app::MouseEvent &event);
	void										sendMouseDragEvent(const ci::app::MouseEvent &event);
	void										sendMouseUpEvent(const ci::app::MouseEvent &event);

	void										sendMouseClick(const ci::Vec3f& globalClickPoint);

	// DEPRECATED: This is for API-compatibility with the old Awesomium. Always draws while loading now.
	void										setDrawWhileLoading(const bool){};


	// Set the zoom level, where 1 = 100%, 0.25 = 25% etc.
	// Note: this is not the same value as CEF zoom levels (where 0.0 == 100%). This is percentage, like Chrome
	void										setZoom(const double percent);
	// Get the zoom level, where 1 = 100%, 0.25 = 25% etc.
	// Note: this is not the same value as CEF zoom levels (where 0.0 == 100%). This is percentage, like Chrome
	double										getZoom() const;

	// Actions
	void										goBack();
	void										goForward();
	void										reload(const bool ignoreCache = false);
	bool										isLoading();
	void										stop();
	bool										canGoBack();
	bool										canGoForward();

	const std::wstring&							getPageTitle(){ return mTitle; }

	//--- Page Callbacks ----------------------------------- //
	// The page's title has been updated (this may happen multiple times per page load)
	void										setTitleChangedFn(const std::function<void(const std::wstring& newTitle)>&);

	// The page has been navigated to a new address
	void										setAddressChangedFn(const std::function<void(const std::string& new_address)>&);

	// The state of loading in the web browser has been updated (started or finished loading)
	// This is a great time to check the canGoBack(), canGoForwards(), and getURL() as they'll be updated immediately before this
	// If loading just completed, the DocumentReadyFn will be called immediately after this 
	void										setLoadingUpdatedCallback(std::function<void(const bool isLoading)> func);

	// This API is for compatibility with older code that only got updated when the load was complete. Use loadingUpdatedCallback for more granular updates
	// The page has finished loading. This also updates the canNext / canBack properties
	void										setDocumentReadyFn(const std::function<void(void)>&);

	// Something went wrong and the page couldn't load. 
	// Passes back a string with some info (should probably pass back a more complete package of info at some point)
	void										setErrorCallback(std::function<void(const std::string&)> func);

	// The page entered or exited fullscreen. The bool will be true if in fullscreen. 
	// The content that's fullscreen'ed will take up the entire web instance. 
	void										setFullscreenChangedCallback(std::function<void(const bool)> func);

	// An error has occurred. No longer displays a text sprite for errors, simply calls back the error callback.
	// You're responsible for displaying the error message yourself
	void										setErrorMessage(const std::string &message);
	void										clearError();

	// Convenience to access various document properties. Note that
	// the document probably needs to have passed onLoaded() for this
	// to be reliable.
	ci::Vec2f									getDocumentSize();
	ci::Vec2f									getDocumentScroll();

	// Scripting.
	// Send function to object with supplied args. For example, if you want to just invoke the global
	// function "makeItHappen()" you'd call: RunJavaScript("window", "makeItHappen", ds::web::ScriptTree());
	//ds::web::ScriptTree		runJavaScript(	const std::string& object, const std::string& function,
	//										const ds::web::ScriptTree& args);
	// Register a handler for a callback from javascript
	//void					registerJavaScriptMethod(	const std::string& class_name, const std::string& method_name,
	//													const std::function<void(const ds::web::ScriptTree&)>&);

	void										executeJavascript(const std::string& theScript);

	/// Lets you disable clicking, but still scroll via "mouse wheel"
	void										setAllowClicks(const bool doAllowClicks);

	/// If true, any transparent web pages will be blank, false will have a white background for pages
	void										setWebTransparent(const bool isTransparent);
	bool										getWebTransparent(){ return mTransparentBackground; }


	virtual void								updateClient(const ds::UpdateParams&);
	virtual void								updateServer(const ds::UpdateParams&);
	virtual void								drawLocalClient();

	// DEPRECATED - there's no loading icon anymore
	void										setLoadingIconOpacity(const float opacity){}

protected:
	virtual void								onSizeChanged();
	virtual void								writeAttributesTo(ds::DataBuffer&);
	virtual void								readAttributeFrom(const char attributeId, ds::DataBuffer&);

private:

	// For syncing touch input across client/servers
	struct WebTouch {
		WebTouch(const int x, const int y, const int btn = 0, const int state = 0, const int clickCnt = 0)
		: mX(x), mY(y), mBttn(btn), mState(state), mClickCount(clickCnt), mIsWheel(false), mXDelta(0), mYDelta(0){}
		int mX;
		int mY;
		int mBttn;
		int mClickCount;
		int mState;
		bool mIsWheel;
		int mXDelta;
		int mYDelta;
	};

	// For syncing keyboard input across server/clients
	struct WebKeyboardInput {
		WebKeyboardInput(const int state, const int nativeKeyCode, const char character, const bool shiftDown, const bool controlDown, const bool altDown)
		: mState(state), mNativeKeyCode(nativeKeyCode), mCharacter(character), mShiftDown(shiftDown), mCntrlDown(controlDown), mAltDown(altDown){}
		const int mState;
		const int mNativeKeyCode;
		const char mCharacter;
		const bool mShiftDown;
		const bool mCntrlDown;
		const bool mAltDown;
	};

	// For syncing back/forward/stop/reload across server/clients
	struct WebControl {
		static const int GO_BACK = 0;
		static const int GO_FORW = 1;
		static const int RELOAD_SOFT = 2;
		static const int RELOAD_HARD = 3;
		static const int STOP_LOAD = 4;
		WebControl(const int command) : mCommand(command){}
		const int mCommand;
	};

	// Sends to the local web service as well as syncing to any clients
	void										sendTouchToService(const int xp, const int yp, const int btn, const int state, const int clickCnt, 
																   const bool isWheel = false, const int xDelta = 0, const int yDelta = 0);
	void										update(const ds::UpdateParams&);
	void										handleTouch(const ds::ui::TouchInfo&);

	void										clearBrowser();
	void										createBrowser();
	void										initializeBrowser();
	bool										mNeedsInitialized;

	ds::web::WebCefService&						mService;

	int											mBrowserId;
	unsigned char *								mBuffer;
	bool										mHasBuffer;
	ci::Vec2i									mBrowserSize; // basically the w/h of this sprite, but tracked so we only recreate the buffer when needed
	ci::gl::Texture								mWebTexture;
	bool										mTransparentBackground;

	double										mZoom;
	bool										mNeedsZoomCheck;

	ci::Vec3f									mPreviousTouchPos;
	bool										mAllowClicks;
	bool										mClickDown;
	bool										mDragScrolling;
	int											mDragScrollMinFingers;
	// Cache the page size and scroll during touch events
	ci::Vec2f									mPageSizeCache,
												mPageScrollCache;
	// Prevent the scroll from being cached more than once in an update.
	int32_t										mPageScrollCount;

	// Callbacks need to be called outside of the update loop (since there could be sprites added or removed as a result of the callbacks)
	// So store the callback state and call it using a cinder tween delay
	void										dispatchCallbacks();
	bool										mHasCallbacks;
	bool										mHasDocCallback;
	bool										mHasErrorCallback;
	bool										mHasAddressCallback;
	bool										mHasTitleCallback;
	bool										mHasFullCallback;
	bool										mHasLoadingCallback;

	std::function<void(void)>					mDocumentReadyFn;
	std::function<void(const std::string&)>		mErrorCallback;
	std::function<void(const std::string&)>		mAddressChangedCallback;
	std::function<void(const std::wstring&)>	mTitleChangedCallback;
	std::function<void(const bool)>				mFullscreenCallback;
	std::function<void(const bool)>				mLoadingUpdatedCallback;


	// Replicated state
	std::string									mUrl;
	std::string									mCurrentUrl;
	std::vector<WebTouch>						mTouches;
	std::vector<WebKeyboardInput>				mKeyPresses;
	std::vector<WebControl>						mHistoryRequests;

	bool										mHasError;
	std::string									mErrorMessage;

	// CEF state, cached here so we don't need to (or can't) query the other threads/processes
	std::wstring								mTitle;
	bool										mIsLoading;
	bool										mCanBack;
	bool										mCanForward;
	bool										mIsFullscreen;

	// Ensure threads are locked when getting callbacks, copying buffers, etc
	std::mutex									mMutex;

	ci::CueRef									mCallbacksCue;


	// Initialization
public:
	static void									installAsServer(ds::BlobRegistry&);
	static void									installAsClient(ds::BlobRegistry&);
};

} // namespace ui
} // namespace ds

#endif // DS_UI_SPRITE_WEB_H_