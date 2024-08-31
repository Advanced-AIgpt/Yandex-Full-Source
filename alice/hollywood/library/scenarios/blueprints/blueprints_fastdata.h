#pragma once

#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>
#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints_fastdata.pb.h>

#include <alice/hollywood/library/framework/framework.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywoodFw::NBlueprints {

class TBlueprintsFastData : public IFastData {
public:
    TBlueprintsFastData(const TBlueprintsFastDataProto& proto)
        : FastData_(proto)
    {
    }
    TMaybe<TBlueprintsArgs> Match(const TRunRequest& runRequest, const TStorage& storage) const;

private:
    bool MatchGroup(const TBlueprintsFastDataProto::TBlueprintsGroup& group,
                    const TRunRequest& runRequest,
                    const TStorage& storage) const;

private:
    const TBlueprintsFastDataProto& FastData_;
};

} // namespace NAlice::NHollywoodFw::NBlueprints
