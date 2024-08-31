#include "blueprints_fastdata.h"

namespace NAlice::NHollywoodFw::NBlueprints {

/*
    Check and match runRequest with specified conditions
    TODO: this function will be implemented later
*/
TMaybe<TBlueprintsArgs> TBlueprintsFastData::Match(const TRunRequest& runRequest, const TStorage& storage) const {
    for (const auto& it : FastData_.GetAllBlueprintGroups()) {
        if (!MatchGroup(it, runRequest, storage)) {
            continue;
        }
        // TOOD: Match request with all blueprints inside group
    }
    return Nothing();
}

/*
    Check initial conditions in blueprint group
    TODO: this functuion will be implemented later
*/
bool TBlueprintsFastData::MatchGroup(const TBlueprintsFastDataProto::TBlueprintsGroup& group,
        const TRunRequest& runRequest,
        const TStorage& storage) const {
    Y_UNUSED(runRequest);
    Y_UNUSED(storage);
    Y_UNUSED(group);
    return false;
}

} // namespace NAlice::NHollywoodFw::NBlueprints
