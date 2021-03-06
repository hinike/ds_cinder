#include "stdafx.h"
#if 0
#include "settings_manager.h"

#include <cinder/Xml.h>
#include <Poco/File.h>
#include <Poco/String.h>
#include "ds/app/engine/engine.h"
#include "ds/debug/logger.h"
#include "ds/debug/debug_defines.h"
#include "ds/util/string_util.h"
#include "ds/util/color_util.h"
#include "ds/util/file_meta_data.h"

namespace ds {
namespace cfg {

const std::string&				SETTING_TYPE_UNKNOWN = "unknown";
const std::string&				SETTING_TYPE_BOOL = "bool";
const std::string&				SETTING_TYPE_INT = "int";
const std::string&				SETTING_TYPE_FLOAT = "float";
const std::string&				SETTING_TYPE_DOUBLE = "double";
const std::string&				SETTING_TYPE_STRING = "string";
const std::string&				SETTING_TYPE_WSTRING = "wstring";
const std::string&				SETTING_TYPE_COLOR = "color";
const std::string&				SETTING_TYPE_COLORA = "colora";
const std::string&				SETTING_TYPE_VEC2 = "vec2";
const std::string&				SETTING_TYPE_VEC3 = "vec3";
const std::string&				SETTING_TYPE_RECT = "rect";
const std::string&				SETTING_TYPE_SECTION_HEADER = "section_header";

namespace {

static std::vector<std::string>	SETTING_TYPES;
void initialize_types(){
	if(SETTING_TYPES.empty()){
		SETTING_TYPES.emplace_back(SETTING_TYPE_UNKNOWN);
		SETTING_TYPES.emplace_back(SETTING_TYPE_BOOL);
		SETTING_TYPES.emplace_back(SETTING_TYPE_INT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_FLOAT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_DOUBLE);
		SETTING_TYPES.emplace_back(SETTING_TYPE_STRING);
		SETTING_TYPES.emplace_back(SETTING_TYPE_WSTRING);
		SETTING_TYPES.emplace_back(SETTING_TYPE_COLOR);
		SETTING_TYPES.emplace_back(SETTING_TYPE_COLORA);
		SETTING_TYPES.emplace_back(SETTING_TYPE_VEC2);
		SETTING_TYPES.emplace_back(SETTING_TYPE_VEC3);
		SETTING_TYPES.emplace_back(SETTING_TYPE_RECT);
		SETTING_TYPES.emplace_back(SETTING_TYPE_SECTION_HEADER);
	}
}

static void merge_settings(
		  std::vector<std::pair<std::string, std::vector<ds::cfg::Settings::Setting>>>& dst, 
		  const std::vector<std::pair<std::string, std::vector<ds::cfg::Settings::Setting>>>& src){
	for(auto sit : src) {
		bool found = false;
		for (auto& dit : dst){
			if(dit.first == sit.first){
				dit.second = sit.second;
				found = true;
				break;
			}
		}

		if(!found){
			dst.emplace_back(sit);
		}
	}
}

}

bool SettingsManager::Setting::getBool() const {
	return parseBoolean(mRawValue);
}

int SettingsManager::Setting::getInt() const{
	return ds::string_to_int(mRawValue);
}

float SettingsManager::Setting::getFloat() const{
	return ds::string_to_float(mRawValue);
}

double SettingsManager::Setting::getDouble() const{
	return ds::string_to_double(mRawValue);
}

const ci::Color SettingsManager::Setting::getColor(ds::Engine& eng) const{
	return ds::parseColor(mRawValue, eng);
}

const ci::ColorA SettingsManager::Setting::getColorA(ds::Engine& eng) const{
	return ds::parseColor(mRawValue, eng);
}

const std::string& SettingsManager::Setting::getString() const{
	return mRawValue;
}

const std::wstring SettingsManager::Setting::getWString() const{
	return ds::wstr_from_utf8(mRawValue);
}

const ci::vec2 SettingsManager::Setting::getVec2() const {
	return ci::vec2(parseVector(mRawValue));
}

const ci::vec3 SettingsManager::Setting::getVec3() const {
	return parseVector(mRawValue);
}

const cinder::Rectf& SettingsManager::Setting::getRect() const{
	return parseRect(mRawValue);
}

SettingsManager::SettingsManager(ds::Engine& engine)
	: mEngine(engine)
{
	initialize_types();
}

void SettingsManager::readFrom(const std::string& filename, const bool append){
	if(!append) {
		directReadFrom(filename, true);
		return;
	}

	SettingsManager		s(mEngine);
	s.directReadFrom(filename, false);

	merge_settings(mSettings, s.mSettings);
}

void SettingsManager::directReadFrom(const std::string& filename, const bool clearAll){
	if(clearAll) clear();

	if(filename.empty()) {
		return;
	}

	if(!safeFileExistsCheck(filename)){
		return;
	}


	ci::XmlTree xml;
	try{
		xml = ci::XmlTree(ci::loadFile(filename));
	} catch(std::exception& e){
		DS_LOG_WARNING("Exception loading settings from " << filename << " " << e.what());
		return;
	}

	auto xmlEnd = xml.end();
	for(auto it = xml.begin("settings/setting"); it != xmlEnd; ++it){
		if(!it->hasAttribute("name")){
			DS_LOG_WARNING("Missing a name attribute for a setting!");
			continue;
		}

		std::string theName = it->getAttributeValue<std::string>("name");

		Setting theSetting;
		theSetting.mName = theName;
		if(it->hasAttribute("value"))	theSetting.mRawValue = it->getAttributeValue<std::string>("value");
		if(it->hasAttribute("comment"))	theSetting.mComment = it->getAttributeValue<std::string>("comment");
		if(it->hasAttribute("default"))	theSetting.mDefault = it->getAttributeValue<std::string>("default");
		if(it->hasAttribute("min_value"))	theSetting.mMinValue = it->getAttributeValue<std::string>("min_value");
		if(it->hasAttribute("max_value"))	theSetting.mMaxValue = it->getAttributeValue<std::string>("max_value");
		if(it->hasAttribute("type"))	theSetting.mType = it->getAttributeValue<std::string>("type");

		if(!validateType(theSetting.mType)){
			DS_LOG_WARNING("Unknown setting type for " << theName << " type:" << theSetting.mType << " source: " << filename);
		}

		theSetting.mSource = filename;

		auto settingIndex = getSettingIndex(theName);
		if(settingIndex > -1 && !mSettings.empty()){
			mSettings[settingIndex].second.push_back(theSetting);
		} else {
			std::vector<Setting> newSettingVec;
			newSettingVec.push_back(theSetting);
			mSettings.emplace_back(std::pair<std::string, std::vector<Setting>>(theName, newSettingVec));
		}
	}
}


void SettingsManager::writeTo(const std::string& filename){

	ci::XmlTree xml = ci::XmlTree::createDoc();
	ci::XmlTree rootNode;
	rootNode.setTag("settings");
	for (auto it : mSettings){
		for(auto sit : it.second){
			ci::XmlTree settingNode;
			settingNode.setTag("setting");
			settingNode.setAttribute("name", sit.mName);
			settingNode.setAttribute("value", sit.mRawValue);
			if(!sit.mType.empty()) settingNode.setAttribute("type", sit.mType);
			if(!sit.mComment.empty()) settingNode.setAttribute("comment", sit.mComment);
			if(!sit.mDefault.empty()) settingNode.setAttribute("default", sit.mDefault);
			if(!sit.mMinValue.empty()) settingNode.setAttribute("min_value", sit.mMinValue);
			if(!sit.mMaxValue.empty()) settingNode.setAttribute("max_value", sit.mMaxValue);
			rootNode.push_back(settingNode);
		}
	}

	xml.push_back(rootNode);
	xml.write(ci::writeFile(filename), true);
}

bool SettingsManager::empty() const {
	return mSettings.empty();
}

void SettingsManager::clear() {
	mSettings.clear();
}


const bool SettingsManager::getBool(const std::string& name, const int index) {
	return getSetting(name, index).getBool();
}

const bool SettingsManager::getBool(const std::string& name, const int index, const bool defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getBool();
}

const int SettingsManager::getInt(const std::string& name, const int index){
	return getSetting(name, index).getInt();
}

const int SettingsManager::getInt(const std::string& name, const int index, const int defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getInt();
}

const float SettingsManager::getFloat(const std::string& name, const int index){
	return getSetting(name, index).getFloat();
}

const float SettingsManager::getFloat(const std::string& name, const int index, const float defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getFloat();
}

const double SettingsManager::getDouble(const std::string& name, const int index){
	return getSetting(name, index).getDouble();
}

const double SettingsManager::getDouble(const std::string& name, const int index, const double defaultValue){
	return getSetting(name, index, std::to_string(defaultValue)).getDouble();
}

const ci::Color SettingsManager::getColor(const std::string& name, const int index){
	return getSetting(name, index).getColor(mEngine);
}

const ci::Color SettingsManager::getColor(const std::string& name, const int index, const ci::Color& defaultValue){
	return getSetting(name, index, ds::unparseColor(defaultValue)).getColor(mEngine);
}

const ci::ColorA SettingsManager::getColorA(const std::string& name, const int index){
	return getSetting(name, index).getColorA(mEngine);
}

const ci::ColorA SettingsManager::getColorA(const std::string& name, const int index, const ci::ColorA& defaultValue){
	return getSetting(name, index, ds::unparseColor(defaultValue)).getColor(mEngine);
}

const std::string& SettingsManager::getString(const std::string& name, const int index){
	return getSetting(name, index).getString();
}

const std::string& SettingsManager::getString(const std::string& name, const int index, const std::string& defaultValue){
	return getSetting(name, index, defaultValue).getString();
}

const std::wstring SettingsManager::getWString(const std::string& name, const int index){
	return getSetting(name, index).getWString();
}

const std::wstring SettingsManager::getWString(const std::string& name, const int index, const std::wstring& defaultValue){
	return getSetting(name, index, ds::utf8_from_wstr(defaultValue)).getWString();
}

const ci::vec2& SettingsManager::getVec2(const std::string& name, const int index){
	return getSetting(name, index).getVec2();
}

const ci::vec2& SettingsManager::getVec2(const std::string& name, const int index, const ci::vec2& defaultValue){
	return getSetting(name, index, ds::unparseVector(defaultValue)).getVec2();
}

const ci::vec3& SettingsManager::getVec3(const std::string& name, const int index){
	return getSetting(name, index).getVec3();
}

const ci::vec3& SettingsManager::getVec3(const std::string& name, const int index, const ci::vec3& defaultValue){
	return getSetting(name, index, ds::unparseVector(defaultValue)).getVec3();
}

const cinder::Rectf& SettingsManager::getRect(const std::string& name, const int index){
	return getSetting(name, index).getRect();
}

const cinder::Rectf& SettingsManager::getRect(const std::string& name, const int index, const ci::Rectf& defaultValue){
	return getSetting(name, index, ds::unparseRect(defaultValue)).getRect();
}

bool SettingsManager::validateType(const std::string& inputType) {
	return std::find(SETTING_TYPES.begin(), SETTING_TYPES.end(), inputType) != SETTING_TYPES.end();
}

bool SettingsManager::hasSetting(const std::string& name) const {
	for (auto it : mSettings){
		if(it.first == name) return true;
	}

	return false;
}

int SettingsManager::getSettingIndex(const std::string& name) const {
	for(int i = 0; i < mSettings.size(); i++){
		if(mSettings[i].first == name) return i;
	}

	return -1;
}

void SettingsManager::forEachSetting(const std::function<void(const Setting&)>& func, const std::string& typeFilter /*= ""*/) const{
	for (auto it : mSettings){
		auto theThingies = it.second;
		for (auto sit : theThingies){
			if(!typeFilter.empty() && typeFilter != sit.mType) continue;
			func(sit);
		}
	}
}

ds::cfg::SettingsManager::Setting& SettingsManager::getSetting(const std::string& name, const int index) {
	return getSetting(name, index, "");
}

ds::cfg::SettingsManager::Setting& SettingsManager::getSetting(const std::string& name, const int index, const std::string& defaultRawValue){
	auto settingIndex = getSettingIndex(name);

	if(settingIndex > -1 && index > -1 && !mSettings[settingIndex].second.empty() && index < mSettings[settingIndex].second.size()){
		return mSettings[settingIndex].second[index];
	}

	// create a new blank setting and return that
	std::vector<Setting> settings;
	settings.push_back(Setting());
	settings.back().mName = name;
	mSettings.emplace_back(std::pair<std::string, std::vector<Setting>>(name, settings));
	return mSettings.back().second.back();

}

void SettingsManager::addSetting(const Setting& newSetting){
	auto settingIndex = getSettingIndex(newSetting.mName);

	if(settingIndex > -1 && !mSettings.empty()){
		mSettings[settingIndex].second.push_back(newSetting);
	} else {
		std::vector<Setting> theSettings;
		theSettings.push_back(newSetting);
		auto hintInsert = mSettings.end();
		mSettings.insert(hintInsert, std::pair<std::string, std::vector<Setting>>(newSetting.mName, theSettings));
	}
}

void SettingsManager::printAllSettings(){
	std::cout << "Settings: " << std::endl;
	for (auto it : mSettings){
		auto theVec = it.second;
		for(auto sit : theVec){
			std::cout << std::endl << "\t" << sit.mName << ": \t" << sit.mRawValue << std::endl;
			std::cout << "\t\t source: \t" << sit.mSource << std::endl;
			
			if(!sit.mComment.empty()) std::cout << "\t\t comment: \t" << sit.mComment << std::endl;
			if(!sit.mType.empty()) std::cout << "\t\t type: \t\t" << sit.mType << std::endl;
			if(!sit.mDefault.empty()) std::cout << "\t\t default: \t" << sit.mDefault << std::endl;
			if(!sit.mMinValue.empty()) std::cout << "\t\t min: \t\t" << sit.mMinValue << std::endl;
			if(!sit.mMaxValue.empty()) std::cout << "\t\t max: \t\t" << sit.mMaxValue << std::endl;
		}
	}
}

} // namespace cfg
} // namespace ds

#endif