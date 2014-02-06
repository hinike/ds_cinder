#pragma once
#ifndef DS_UI_SPRITE_GST_VIDEO_H_
#define DS_UI_SPRITE_GST_VIDEO_H_

#include <ds/ui/sprite/sprite.h>
#include <string>
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "ds/data/resource.h"

namespace _2RealGStreamerWrapper {
class GStreamerWrapper;
}

namespace ds {
namespace ui {

/**
 * \class ds::ui::GstVideo
 * \brief Video playback.
 * Note: If you want to use multiple playback systems simultaneously, use this
 * uniquely-named class. If you want to have one playback system that you can
 * swap out easily, include "video.h" and just use the Video class.
 */
class GstVideo : public Sprite {
public:
	GstVideo(SpriteEngine&);
	~GstVideo();
	void				setAlphaMode(bool isTransparent);// set this before loading a video
	void				setSize( float width, float height );
	virtual void		updateServer(const UpdateParams&);
	void				drawLocalClient();

	GstVideo&			loadVideo( const std::string &filename);
	GstVideo&			setResourceId(const ds::Resource::Id &resourceId);
	void				unloadVideo();

	// Setup
	void				setLooping(const bool on);
	bool				getIsLooping() const;
	void				setMute(const bool on);
	bool				getIsMuted() const;
	// value between 0.0f and 1.0f
	void				setVolume(const float volume);
	float				getVolume() const;

	// Commands
	void				play();
	void				stop();
	void				pause();
	bool				getIsPlaying() const;

	// Time operations (in seconds)
	double				getDuration() const;
	double				getCurrentTime() const;
	void				seekTime(const double);

    // Position operations (in unit values, 0 - 1)
	double				getCurrentPosition() const;
	void				seekPosition(const double);

	struct Status {
		static const int  STATUS_STOPPED = 0;
		static const int  STATUS_PLAYING = 1;
		static const int  STATUS_PAUSED  = 2;
		int               mCode;
	};
	void				setStatusCallback(const std::function<void(const Status&)>&);

	void				setVideoCompleteCallback(const std::function<void(GstVideo* video)> &func);

	// If true, will play the video as soon as it's loaded.
	void				setAutoStart(const bool doAutoStart);

	// Set's the video to play, then stops the video after that frame has played.
	// Optionally supply a function called once I've played a frame.
	void				playAFrame(const std::function<void(GstVideo&)>& fn = nullptr);
	bool				isPlayingAFrame() const;
	// If a video is looping, will stop the video when the current loop completes.
	void				stopAfterNextLoop();
	
private:
	typedef Sprite		inherited;

	void                setStatus(const int);
	void				setMovieVolume();
	void				handleVideoComplete(_2RealGStreamerWrapper::GStreamerWrapper*);

	// Done this way so I can completely hide any dependencies
	_2RealGStreamerWrapper::GStreamerWrapper*	mMoviePtr;
	_2RealGStreamerWrapper::GStreamerWrapper&	mMovie;

	ci::gl::Texture     mFrameTexture;
	ci::gl::Fbo         mFbo;

	bool                mLooping;
	// User-driven mute state
	bool				mMuted;
	// A mute state that gets turned on automatically in certain situations
	bool                mInternalMuted;
	float               mVolume;
	bool				mIsTransparent;

	Status              mStatus;
	bool                mStatusDirty;

	bool				mPlaySingleFrame;
	std::function<void(GstVideo&)>
						mPlaySingleFrameFn;

	std::function<void(const Status&)>
						mStatusFn;
	std::function<void(GstVideo*)>
						mVideoCompleteCallback;
};

} // namespace ui
} // namespace ds

#endif // DS_UI_SPRITE_GST_VIDEO_H_
