#pragma once
#ifndef PRIVATE_CEF_WEB_CALLBACKS_H
#define PRIVATE_CEF_WEB_CALLBACKS_H

#include <functional>

namespace ds {
class Engine;

namespace web {

/**
* \class ds::web::WebCefCallbacks
* \brief A wrapper object for all the callbacks from CEF to Web sprites
*/
struct WebCefCallbacks  {
public:
	WebCefCallbacks(){};

	// Gets called when the browser sends new paint info, aka new buffers
	std::function<void(const void *, const int, const int)> mPaintCallback;

	// The loading state has changed (started or finished loading.
	std::function<void(const bool isLoading, const bool canGoBack, const bool canGoForward, const std::string& newUrl)> mLoadChangeCallback;

	// The title of the page has changed
	std::function<void(const std::wstring&)>				mTitleChangeCallback;

	// Something bad happened!
	std::function<void(const std::string&)>					mErrorCallback;

	std::function<void(const bool)>							mFullscreenCallback;



};

} // namespace web
} // namespace ds

#endif // PRIVATE_CEF_WEB_CALLBACKS_H