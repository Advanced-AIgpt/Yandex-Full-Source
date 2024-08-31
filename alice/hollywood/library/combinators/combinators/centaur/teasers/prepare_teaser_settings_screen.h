#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/combinator_context_wrapper.h>
#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>

namespace NAlice::NHollywood::NCombinators { 

namespace NMemento = ru::yandex::alice::memento::proto;

using DirectivePointer = google::protobuf::internal::RepeatedPtrIterator<const NScenarios::TDirective>;

class TPrepareTeaserSettingsScreen {
public:
    TPrepareTeaserSettingsScreen(THwServiceContext& ctx, TCombinatorRequestWrapper& combinatorRequest);
    void Do();

private:
    void MakeScenarioResponse(const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig);
    TString GetTeaserName(const TString& teaserType);

    THwServiceContext& Ctx;
    THashSet<TString> UsedScenarios;
    TCombinatorRequestWrapper& Request;
    TCombinatorContextWrapper CombinatorContextWrapper;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
};

}
