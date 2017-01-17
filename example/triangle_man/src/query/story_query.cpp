#include "story_query.h"

#include <map>
#include <sstream>
#include <ds/debug/logger.h>
#include <ds/query/query_client.h>

#include <ds/app/environment.h>

namespace nwm {

/**
 * \class nwm::StoryQuery
 */
StoryQuery::StoryQuery() {
}

void StoryQuery::run() {
	mOutput.mStories.clear();
	try {
		query(mOutput);
	} catch (std::exception const&) {
	}
}

void StoryQuery::query(AllStories& output) {
	//output.mIndustries.push_back(IndustryModelRef())


}

} // !namespace nwm


