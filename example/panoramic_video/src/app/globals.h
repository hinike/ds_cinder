#ifndef _PANORAMICVIDEO_APP_GLOBALS_
#define _PANORAMICVIDEO_APP_GLOBALS_

#include <ds/cfg/cfg_text.h>
#include <ds/cfg/settings.h>
#include <ds/ui/sprite/sprite_engine.h>

#include "model/all_data.h"

namespace ds {
namespace ui {
class SpriteEngine;
} // namespace ui
} // namespace ds

namespace panoramic {

/**
 * \class panoramic::Globals
 * \brief Global data for the app.
 */
class Globals {
public:
	Globals(ds::ui::SpriteEngine&, const AllData& d);

	ds::ui::SpriteEngine&			mEngine;

	const AllData&					mAllData;

	const float						getAnimDur();

	void							initialize();

	//Shortcuts
	const ds::cfg::Text&			getText(const std::string& name) const;
	ds::cfg::Settings&				getSettingsLayout() const;
	ds::cfg::Settings&				getSettings(const std::string& name) const;

private:

	float							mAnimationDuration;
};

} // !namespace panoramic

#endif // !_PANORAMICVIDEO_APP_GLOBALS_

