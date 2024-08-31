#include "postclassify_state.h"

#include <alice/megamind/library/apphost_request/protos/combinators.pb.h>
#include <alice/megamind/library/apphost_request/util.h>

namespace NAlice::NMegamind {

TPostClassifyState::TPostClassifyState(TItemProxyAdapter& itemAdapter)
    : ItemAdapter{itemAdapter} {
}

TErrorOr<TString> TPostClassifyState::GetWinnerScenario() {
    auto winnerScenarioProto =
        ItemAdapter.GetFromContext<NMegamindAppHost::TScenarioProto>(NMegamind::AH_ITEM_WINNER_SCENARIO);
    if (winnerScenarioProto.Error()) {
        return std::move(*winnerScenarioProto.Error());
    }
    return winnerScenarioProto.Value().GetName();
}

TErrorOr<TMegamindAnalyticsInfo> TPostClassifyState::GetAnalytics() {
    return ItemAdapter.GetFromContext<NMegamind::TMegamindAnalyticsInfo>(NMegamind::AH_ITEM_ANALYTICS_POSTCLASSIFY);
}

TErrorOr<TQualityStorage> TPostClassifyState::GetQualityStorage() {
    return ItemAdapter.GetFromContext<TQualityStorage>(NMegamind::AH_ITEM_QUALITYSTORAGE_POSTCLASSIFY);
}

TMaybe<NMegamindAppHost::TScenarioErrorsProto> TPostClassifyState::GetScenarioErrors() {
    auto scenarioErrors =
        ItemAdapter.GetFromContext<NMegamindAppHost::TScenarioErrorsProto>(NMegamind::AH_ITEM_SCENARIO_ERRORS);
    if (scenarioErrors.Error()) {
        return Nothing();
    }
    return scenarioErrors.Value();
}

TStatus TPostClassifyState::GetPostClassifyStatus() {
    auto errorProto = ItemAdapter.GetFromContext<NMegamindAppHost::TErrorProto>(NMegamind::AH_ITEM_ERROR_POSTCLASSIFY);
    if (errorProto.Error()) { // It means no error in walker_run_postclassify so status is ok
        return Success();
    }
    return NMegamind::ErrorFromProto(errorProto.Value());
}

TMaybe<TString> TPostClassifyState::GetWinnerCombinator() {
    auto proto = ItemAdapter.GetFromContext<NMegamindAppHost::TCombinatorProto>(AH_ITEM_WINNER_COMBINATOR);
    if (proto.Error()) {
        return Nothing();
    }
    return proto.Value().GetName();
}

TMaybe<NMegamindAppHost::TCombinatorProto::ECombinatorStage> TPostClassifyState::GetWinnerCombinatorStage() {
    auto proto = ItemAdapter.GetFromContext<NMegamindAppHost::TCombinatorProto>(AH_ITEM_WINNER_COMBINATOR);
    if (proto.Error()) {
        return Nothing();
    }
    return proto.Value().GetStage();
}

TMaybe<NScenarios::TScenarioContinueResponse> TPostClassifyState::GetContinueResponse() {
    auto proto =
        ItemAdapter.GetFromContext<NScenarios::TScenarioContinueResponse>(AH_ITEM_CONTINUE_RESPONSE_POSTCLASSIFY);
    if (proto.Error()) {
        return Nothing();
    }
    return std::move(proto.Value());
}

} // namespace NAlice::NMegamind
