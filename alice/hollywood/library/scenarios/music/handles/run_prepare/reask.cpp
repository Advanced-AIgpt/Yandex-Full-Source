#include "impl.h"

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

constexpr uint MAX_REASK_COUNT = 2;

} // namespace

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleReask() {
    const bool supportsReask = UserInfo_ && UserInfo_->GetHasYandexPlus() &&
        Request_.ClientInfo().IsSmartSpeaker() &&
        !Request_.HasExpFlag(NExperiments::EXP_MUSIC_DISABLE_REASK);
    if (!supportsReask) {
        return Nothing();
    }

    ui32 reaskCount = 0;
    if (ScenarioState_ && !Request_.IsNewSession()) {
        reaskCount = ScenarioState_->GetReaskState().GetReaskCount();
    }

    // TODO(ardulat): manage how frequently we would like to clarify
    if (const auto missingType = Frame_.FindSlot(NAlice::NMusic::SLOT_MISSING_TYPE);
        missingType && reaskCount < MAX_REASK_COUNT)
    {
        THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
        auto& bodyBuilder = response.CreateResponseBodyBuilder();

        TMusicReaskState& reaskState = *GetScenarioStateOrDummy().MutableReaskState();
        TMusicReaskState::EMissingType value;
        if (TMusicReaskState::EMissingType_Parse(missingType->Value.AsString(), &value)) {
            reaskState.SetMissingType(value);
        }
        reaskState.SetReaskCount(reaskCount + 1);
        bodyBuilder.SetState(GetScenarioStateOrDummy());
        bodyBuilder.SetExpectsRequest(true);
        bodyBuilder.SetShouldListen(true);

        TNlgData nlgData{Logger_, Request_};
        nlgData.Context["missing_type"] = missingType->Value.AsString();
        nlgData.Context["reask_count"] = reaskCount;
        auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsInfoBuilder.SetIntentName("music.reask");
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "ask_again",
                                                       /* buttons = */ {}, nlgData);
        return *std::move(response).BuildResponse();
    } else if (Frame_.FindSlot(NAlice::NMusic::SLOT_DECLINE)) {
        return CreateIrrelevantResponseMusicNotFound();
    } else if (const auto searchText = Frame_.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT);
               searchText && GetScenarioStateOrDummy().HasReaskState())
    {
        TString prefix = "";
        const auto missingType = GetScenarioStateOrDummy().GetReaskState().GetMissingType();
        switch (missingType) {
            case TMusicReaskState::Track:
                prefix = "песня";
                break;
            case TMusicReaskState::Album:
                prefix = "альбом";
                break;
            case TMusicReaskState::Artist:
                prefix = "исполнитель";
                break;
            default:
                break;
        }
        AddPrefixToSearchText(prefix, *searchText);
        // NOTICE!! searchText is invalid after this
    }

    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
