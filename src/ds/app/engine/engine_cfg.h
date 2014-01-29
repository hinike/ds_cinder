#pragma once
#ifndef DS_APP_ENGINE_ENGINECFG_H_
#define DS_APP_ENGINE_ENGINECFG_H_

#include <unordered_map>
#include "ds/cfg/cfg_nine_patch.h"
#include "ds/cfg/cfg_text.h"
#include "ds/cfg/settings.h"

namespace ds {

/**
 * \class ds::EngineCfg
 * \brief Store all the engine configuration info.
 */
class EngineCfg {
public:
	EngineCfg();

	// Answer the requested settings file. In debug mode, throw
	// if it doesn't exist. In release mode, just answer an empty one.
	const ds::cfg::Settings&		getSettings(const std::string& name) const;
	ds::cfg::Settings&				editSettings(const std::string& name);
	// Answer the requested text cfg. In debug mode, throw
	// if it doesn't exist. In release mode, just answer an empty one.
	const ds::cfg::Text&			getText(const std::string& name) const;
	bool							hasText(const std::string& name) const;
	// Answer the requested text cfg. In debug mode, throw
	// if it doesn't exist. In release mode, just answer an empty one.
	const ds::cfg::NinePatch&		getNinePatch(const std::string&) const;
	bool							hasNinePatch(const std::string& name) const;

	// Convenice to load a setting file into the mEngineCfg settings.
	// @param name is the name that the system will use to refer to the settings.
	// @param filename is the leaf path of the settings file (i.e. "data.xml").
	// It will be loaded from all appropriate locations.
	void							loadSettings(const std::string& name, const std::string& filename);
	// Convenice to load a text cfg file into a collection of cfg objects.
	// @param filename is the leaf path of the settings file (i.e. "text.xml").
	// It will be loaded from all appropriate locations.
	void							loadText(const std::string& filename);
	// Convenice to load a nine patch cfg file into a collection of cfg objects.
	// @param filename is the leaf path of the settings file (i.e. "nine_patch.xml").
	// It will be loaded from all appropriate locations.
	void							loadNinePatchCfg(const std::string& filename);

private:
	EngineCfg(const EngineCfg&);

	std::unordered_map<std::string, ds::cfg::Settings>
									mSettings;
	std::unordered_map<std::string, ds::cfg::Text>
									mTextCfg;
	std::unordered_map<std::string, ds::cfg::NinePatch>
									mNinePatchCfg;
};

} // namespace ds

#endif // DS_APP_ENGINE_ENGINECFG_H_
