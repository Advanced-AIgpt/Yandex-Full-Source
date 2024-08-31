#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/combinator_context_wrapper.h>
#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>

namespace NAlice::NHollywood::NCombinators { 

namespace NMemento = ru::yandex::alice::memento::proto;

using DirectivePointer = google::protobuf::internal::RepeatedPtrIterator<const NScenarios::TDirective>;
using AddDirectivePointer = TVector<NAlice::NScenarios::TAddCardDirective>::const_iterator;

class TPrepareTeasers {
public:
    TPrepareTeasers(THwServiceContext& Ctx, TCombinatorRequestWrapper& combinatorRequest);
    void Do();

private:

    void MakeScenarioResponse();
    
    void MakeScenarioResponseWithSettings(const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig);
    
    template <typename TScenarioResponse>
    void AddScenarioResponse(
        const TScenarioResponse& scenarioResponse,
        THashMap<TString, std::pair<DirectivePointer, DirectivePointer>>& teaserStack, 
        const auto& scenarioName
    );
    
    void SetToRenderChromeLayers(const google::protobuf::Map<TString, NScenarios::TScenarioRunResponse>& scenarioResponses);

    TVector<NScenarios::TAddCardDirective> getSortedTeasers(
        TVector<int> sizeVector,
        TVector<AddDirectivePointer> pointers
    ); 
    
    TMaybe<TString> GetCarouselId();

    THwServiceContext& Ctx;
    THashSet<TString> UsedScenarios;
    TCombinatorRequestWrapper& Request;
    TCombinatorContextWrapper CombinatorContextWrapper;
    NScenarios::TScenarioRunResponse ResponseForRenderer;

};

}
