#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

class TSearchOldFlowScene : public TScene<NHollywood::TSearchEmptyProto> {
public:
    TSearchOldFlowScene(const TScenario* owner)
        : TScene(owner, "old_flow")
    {
    }
    TRetMain Main(const NHollywood::TSearchEmptyProto&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // NAlice::NHollywoodFw::NSearch
