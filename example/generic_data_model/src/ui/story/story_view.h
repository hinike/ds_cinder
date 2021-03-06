#pragma once
#ifndef _GENERIC_DATA_MODEL_APP_UI_STORY_STORY_VIEW_H_
#define _GENERIC_DATA_MODEL_APP_UI_STORY_STORY_VIEW_H_


#include <ds/ui/sprite/sprite.h>
#include <ds/app/event_client.h>
#include <ds/ui/sprite/text.h>
#include <ds/ui/sprite/image.h>
#include <ds/ui/layout/layout_sprite.h>

namespace downstream {

class Globals;

/**
* \class downstream::StoryView
*			A sample view
*/
class StoryView final : public ds::ui::Sprite  {
public:
	StoryView(Globals& g);

private:
	void								onAppEvent(const ds::Event&);

	virtual void						onUpdateServer(const ds::UpdateParams& p) override;

	void								animateOn();
	void								animateOff();

	void								setData();

	void								layout();

	Globals&							mGlobals;

	ds::EventClient						mEventClient;
	ds::ui::LayoutSprite*				mPrimaryLayout;
	ds::ui::Text*						mMessage;
	ds::ui::Image*						mImage;

};

} // namespace downstream

#endif

