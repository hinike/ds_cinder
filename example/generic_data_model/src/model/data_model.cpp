#include "stdafx.h"

#include "data_model.h" 

#include <ds/util/string_util.h>
#include <ds/util/color_util.h>


namespace ds {
namespace model {
namespace {
const int							EMPTY_INT = 0;
// const unsigned int					EMPTY_UINT = 0;
// const ci::vec2						EMPTY_VEC2 = ci::vec2();
// const ci::vec3						EMPTY_VEC3 = ci::vec3();
// const ci::Rectf						EMPTY_RECTF = ci::Rectf();
// const float							EMPTY_FLOAT = 0.0f;
// const double						EMPTY_DOUBLE = 0.0;
// const ci::Color						EMPTY_COLOR = ci::Color(0.0f, 0.0f, 0.0f);
// const ci::ColorA					EMPTY_COLORA = ci::ColorA(0.0f, 0.0f, 0.0f, 0.0f);
const std::string					EMPTY_STRING;
const ds::Resource					EMPTY_RESOURCE;
const std::vector<DataModelRef>		EMPTY_DATAMODELREF_VECTOR;
const DataModelRef										EMPTY_DATAMODEL;
const DataProperty										EMPTY_PROPERTY;
const std::map<std::string, DataProperty>				EMPTY_PROPERTY_MAP;
const std::map<std::string, std::vector<DataModelRef>>	EMPTY_CHILDREN_MAP;

}

DataProperty::DataProperty()
	: mName("")
	, mValue("")
	, mResource(EMPTY_RESOURCE)
{
}

DataProperty::DataProperty(const std::string& name, const std::string& value) {
	setValue(value);
	setName(name);
}
const std::string& DataProperty::getName() const {
	return mName;
}

void DataProperty::setName(const std::string& name) {
	mName = name;
}

const std::string& DataProperty::getValue() const {
	return mValue;
}

void DataProperty::setValue(const std::string& value) {
	mValue = value;
}

void DataProperty::setValue(const std::wstring& value) {
	mValue = ds::utf8_from_wstr(value);
}

void DataProperty::setValue(const int& value) {
	mValue = ds::value_to_string<int>(value);
}

void DataProperty::setValue(const double& value) {
	mValue = ds::value_to_string<double>(value);
}

void DataProperty::setValue(const float& value) {
	mValue = ds::value_to_string<float>(value);
}

void DataProperty::setValue(const ci::Color& value) {
	mValue = ds::unparseColor(value);
}

void DataProperty::setValue(const ci::ColorA& value) {
	mValue = ds::unparseColor(value);
}

void DataProperty::setValue(const ci::vec2& value) {
	mValue = ds::unparseVector(value);
}

void DataProperty::setValue(const ci::vec3& value) {
	mValue = ds::unparseVector(value);
}

void DataProperty::setValue(const ci::Rectf& value) {
	mValue = ds::unparseRect(value);
}

ds::Resource DataProperty::getResource() const {
	return mResource;
}

void DataProperty::setResource(const ds::Resource& resource) {
	mResource = resource;
}

bool DataProperty::getBool() const {
	return ds::parseBoolean(mValue);
}

int DataProperty::getInt() const {
	return ds::string_to_int(mValue);
}

float DataProperty::getFloat() const {
	return ds::string_to_float(mValue);
}

double DataProperty::getDouble() const {
	return ds::string_to_double(mValue);
}

const ci::Color DataProperty::getColor(ds::ui::SpriteEngine& eng) const {
	return ds::parseColor(mValue, eng);
}

const ci::ColorA DataProperty::getColorA(ds::ui::SpriteEngine& eng) const {
	return ds::parseColor(mValue, eng);
}

const std::string& DataProperty::getString() const {
	return getValue();
}

const std::wstring DataProperty::getWString() const {
	return ds::wstr_from_utf8(getString());
}

const ci::vec2 DataProperty::getVec2() const {
	return ci::vec2(ds::parseVector(mValue));
}

const ci::vec3 DataProperty::getVec3() const {
	return ds::parseVector(mValue);
}

const ci::Rectf DataProperty::getRect() const {
	return ds::parseRect(mValue);
}



class DataModelRef::Data {
public:
	Data()
		: mName(EMPTY_STRING)
		, mId(EMPTY_INT)
	{}

	std::string mName;
	int mId;
	std::map<std::string, DataProperty> mProperties;
	std::map<std::string, std::vector<DataModelRef>> mChildren;

};

DataModelRef::DataModelRef() {}


DataModelRef::DataModelRef(const std::string& name, const int id) {
	setName(name);
	setId(id);
}

const int& DataModelRef::getId() const {
	if(!mData) return EMPTY_INT;
	return mData->mId;
}

void DataModelRef::setId(const int& id) {
	createData();
	mData->mId = id;
}

const std::string& DataModelRef::getName() const {
	if(!mData) return EMPTY_STRING;
	return mData->mName;
}

void DataModelRef::setName(const std::string& name) {
	createData();
	mData->mName = name;
}

const bool DataModelRef::empty() const {
	if(!mData) return true;
	if(mData->mId == EMPTY_INT
	   && mData->mName == EMPTY_STRING
	   && mData->mChildren.empty()
	   && mData->mProperties.empty()
	   ) {
		return true;
	}

	return false;
}


const std::map<std::string, DataProperty>& DataModelRef::getProperties() {
	if(!mData) return EMPTY_PROPERTY_MAP;
	return mData->mProperties;
}

const ds::model::DataProperty DataModelRef::getProperty(const std::string& propertyName) {
	if(!mData) return EMPTY_PROPERTY;
	auto findy = mData->mProperties.find(propertyName);
	if(findy != mData->mProperties.end()) {
		return findy->second;
	}

	return EMPTY_PROPERTY;
}

const std::string DataModelRef::getPropertyValue(const std::string& propertyName) {
	return getProperty(propertyName).getValue();
}

void DataModelRef::setProperty(const std::string& propertyName, DataProperty datamodel) {
	createData();

	mData->mProperties[propertyName] = datamodel;
}

void DataModelRef::setProperty(const std::string& propertyName, const std::string& propertyValue) {
	createData();
	mData->mProperties[propertyName] = DataProperty(propertyName, propertyValue);
}

const std::map<std::string, std::vector<DataModelRef>>& DataModelRef::getChildrenMap() {
	if(!mData) return EMPTY_CHILDREN_MAP;
	return mData->mChildren;
}

const std::vector<DataModelRef>& DataModelRef::getChildren(const std::string& childrenName) const {
	if(!mData) return EMPTY_DATAMODELREF_VECTOR;
	auto findy = mData->mChildren.find(childrenName);
	if(findy != mData->mChildren.end()) {
		return findy->second;
	}

	return EMPTY_DATAMODELREF_VECTOR;
}

DataModelRef DataModelRef::getChild(const std::string& childName) {
	createData();

	auto findy = mData->mChildren.find(childName);
	if(findy == mData->mChildren.end()
	   || findy->second.empty()
	   ) {
		std::vector<DataModelRef> newChildren;
		newChildren.push_back(DataModelRef(childName));
		mData->mChildren[childName] = newChildren;
		return newChildren.back();
	} 
	// we checked for end and empty above, so this should always be valid
	return findy->second.front();
}

void DataModelRef::setChild(const std::string& childName, DataModelRef datamodel) {
	createData();

	std::vector<DataModelRef> newChildren;
	newChildren.push_back(DataModelRef(childName));
	mData->mChildren[childName] = newChildren;
}

void DataModelRef::addChild(const std::string& childName, DataModelRef datamodel) {
	createData();

	auto& findy = mData->mChildren.find(childName);
	if(findy == mData->mChildren.end()) {
		std::vector<DataModelRef> newChildren;
		newChildren.push_back(datamodel);
		mData->mChildren[childName] = newChildren;
	} else {
		findy->second.emplace_back(datamodel);
	}
}

bool DataModelRef::hasChild(const std::string& childName) {
	if(!mData) return false;
	auto& findy = mData->mChildren.find(childName);
	if(findy != mData->mChildren.end()) {
		return true;
	}

	return false;
}

void DataModelRef::setChildren(const std::string& childrenName, std::vector<ds::model::DataModelRef> children) {
	createData();
	mData->mChildren[childrenName] = children;
}

void DataModelRef::printTree(const bool verbose, const std::string& indent) {
	if(empty() || !mData) {
		DS_LOG_INFO(indent << "Empty DataModel.");
	} else {
		DS_LOG_INFO(indent << "DataModel id:" << mData->mId << " name:" << mData->mName);
		if(verbose) {

			for(auto it : mData->mProperties) {
				if(!it.second.getResource().empty()) {
					DS_LOG_INFO(indent << "          resource:" << it.second.getResource().getAbsoluteFilePath());
				} else {
					DS_LOG_INFO(indent << "          prop:" << it.first << " value:" << it.second.getValue());
				}
			}
		}

		if(!mData->mChildren.empty()) {

			for(auto it : mData->mChildren) {
				DS_LOG_INFO(indent << "          children:" << it.first);
				for(auto cit : it.second) {
					cit.printTree(verbose, indent + "  ");
				}
			}
		}
	}
}

//----------- PRIVATE --------------//
void DataModelRef::createData() {
	if(!mData) mData.reset(new Data());
}

} // namespace model
} // namespace ds
