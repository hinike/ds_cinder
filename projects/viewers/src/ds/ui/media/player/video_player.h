#pragma once
#ifndef DS_UI_MEDIA_VIDEO_PLAYER_VIDEO_PLAYER
#define DS_UI_MEDIA_VIDEO_PLAYER_VIDEO_PLAYER


#include <ds/ui/sprite/sprite.h>

namespace ds {
namespace ui {

class GstVideo;
class VideoInterface;
class MediaInterface;

/**
* \class ds::ui::VideoPlayer
*			Creates a video and puts an interface on top of it.
*/
class VideoPlayer : public ds::ui::Sprite  {
public:
	VideoPlayer(ds::ui::SpriteEngine& eng, const bool embedInterface = true);

	void								setMedia(const std::string mediaPath);

	void								layout();

	void								play();
	void								pause();
	void								stop();

	void								showInterface();

	ds::ui::GstVideo*					getVideo();

protected:

	virtual void						onSizeChanged();
	VideoInterface*						mVideoInterface;
	ds::ui::GstVideo*					mVideo;
	bool								mEmbedInterface;

};

} // namespace ui
} // namespace ds

#endif