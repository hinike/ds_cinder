#pragma once
#ifndef DS_MODEL_AUTOGENERATED_Story
#define DS_MODEL_AUTOGENERATED_Story

#include <ds/data/resource.h>
#include <memory>
#include <vector>
#include <cinder/Vector.h>

#include "media_model.h"
#include "story_model.h"


namespace ds {
namespace model{




/**
* \class ds::model::StoryRef
*			Auto-generated data model for Story
*			Don't manually edit this file. Instead drop the source yaml file onto the yaml_importer in the utility folder of ds_cinder. 
*/
class StoryRef {
public:

	StoryRef();

	const std::wstring& getBody() const;
	const std::wstring& getFileName() const;
	const int& getId() const;
	const ds::Resource& getPrimaryResource() const;
	const std::wstring& getTitle() const;
	const std::wstring& getType() const;
	const std::vector<MediaRef>& getMediaRef() const;
	const std::vector<StoryRef>& getStoryRef() const;


	StoryRef& setBody(const std::wstring& Body);
	StoryRef& setFileName(const std::wstring& FileName);
	StoryRef& setId(const int& Id);
	StoryRef& setPrimaryResource(const ds::Resource& PrimaryResource);
	StoryRef& setTitle(const std::wstring& Title);
	StoryRef& setType(const std::wstring& Type);
	StoryRef& setMediaRef(const std::vector<MediaRef>& MediaRef);
	StoryRef& setStoryRef(const std::vector<StoryRef>& StoryRef);



private:
	class Data;
	std::shared_ptr<Data>	mData;
};

} // namespace model
} // namespace ds

#endif


