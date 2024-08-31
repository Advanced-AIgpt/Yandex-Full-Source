#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/combinator_context_wrapper.h>
#include <alice/hollywood/library/combinators/combinators/centaur/teaser_service.h>

#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>

namespace NAlice::NHollywood::NCombinators { 
    

using DirectivePointer = google::protobuf::internal::RepeatedPtrIterator<const NScenarios::TDirective>;

class TSetTeaserSettings {
public:
    TSetTeaserSettings(THwServiceContext& Ctx, TCombinatorRequestWrapper& combinatorRequest);
    void Do(const TSemanticFrame& semanticFrame);

private:
    NScenarios::TDirective PrepareCollectCardsCallbackDirective();

    THwServiceContext& Ctx;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
    TCombinatorRequestWrapper& Request;
    TCombinatorContextWrapper CombinatorContextWrapper;
};

}
