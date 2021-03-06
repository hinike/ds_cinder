#include "globals.h"

#include <Poco/String.h>

#include <ds/app/engine/engine_cfg.h>
#include <ds/app/environment.h>
#include <ds/cfg/settings.h>

namespace example {

/**
 * \class example::Globals
 */
Globals::Globals(ds::ui::SpriteEngine& e , const AllData& d )
		: mEngine(e)
		, mAllData(d)
{
}


} // !namespace example
