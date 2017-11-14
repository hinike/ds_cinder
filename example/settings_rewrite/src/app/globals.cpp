#include "stdafx.h"

#include "globals.h"

#include <Poco/String.h>

#include <ds/app/engine/engine_cfg.h>
#include <ds/app/environment.h>
#include <ds/cfg/settings.h>

#include "app_defs.h"

namespace downstream {

/**
 * \class downstream::Globals
 */
Globals::Globals(ds::ui::SpriteEngine& e , const AllData& d )
		: mEngine(e)
		, mAllData(d)
		, mAnimationDuration(0.35f)
{
}

const float Globals::getAnimDur(){
	return mAnimationDuration;
}

void Globals::initialize(){
	mAnimationDuration = getAppSettings().getFloat("animation:duration", 0, mAnimationDuration);
}

ds::cfg::Settings& Globals::getSettings(const std::string& name){
	return mEngine.getEngineCfg().getSettings(name);
}

ds::cfg::Settings& Globals::getAppSettings(){
	return mEngine.getEngineCfg().getSettings(SETTINGS_APP);
}


const ds::cfg::Text& Globals::getText(const std::string& name){
	return mEngine.getEngineCfg().getText(name);

}



} // !namespace downstream

