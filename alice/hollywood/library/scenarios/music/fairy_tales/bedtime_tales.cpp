#include "bedtime_tales.h"
#include "semantic_frames.h"

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/proto/music_memento_scenario_data.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString SLOT_FAIRYTALE_THEME = "fairytale_theme";
const TString BEDTIME_FAIRYTALE_SLOT_VALUE = "bedtime";

const int BEDTIME_TALES_ONBOARDING_MAX_COUNTER = 4;

} // namespace

bool IsBedtimeTales(
    const TScenarioInputWrapper& requestInput,
    bool bedtimeTalesExp)
{
    if (!bedtimeTalesExp) {
        return false;
    }
    const auto fairytaleFrameProto = requestInput.FindSemanticFrame(MUSIC_PLAY_FAIRYTALE_FRAME);
    if (fairytaleFrameProto) {
        auto frame = TFrame::FromProto(*fairytaleFrameProto);
        const auto slotFairyTaleTheme = frame.FindSlot(SLOT_FAIRYTALE_THEME);
        return slotFairyTaleTheme && slotFairyTaleTheme->Value.AsString() == BEDTIME_FAIRYTALE_SLOT_VALUE;
    }
    return false;
}

bool CheckFairyTalesStopTimer(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    TScenarioState& scState)
{
    return scState.HasFairytaleTurnOffTimer()
        && (applyRequest.ServerTimeMs() - scState.GetFairytaleTurnOffTimer().GetDuration() > scState.GetFairytaleTurnOffTimer().GetPlayStartedTimestamp());
}

void SetBedtimeTalesState(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    TScenarioState& scState,
    bool isBedtimeTales)
{
    if (isBedtimeTales) {
        scState.MutableFairytaleTurnOffTimer()->SetPlayStartedTimestamp(applyRequest.ServerTimeMs());
        scState.MutableFairytaleTurnOffTimer()->SetDuration(BEDTIME_TALES_PLAY_DURATION_SEC * 1000);
        return;
    }
    scState.ClearFairytaleTurnOffTimer();
}

bool ShouldAddBedtimeTalesOnboarding(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest)
{
    if (!IsBedtimeTales(applyRequest.Input(), applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES))) {
        LOG_INFO(logger) << "Will not add bedtime tales long nlg: it is not bedtime tales request";
        return false;
    }

    if (applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES_FORCE_ONBOARDING)) {
        LOG_INFO(logger) << "Appending bedtime tales onboarding: force onboarding experiment flag presented";
        return true;
    }

    const TMusicScenarioMementoData& mementoScenarioData = ParseMementoData(applyRequest);
    const int bedtimeTalesOnboardingCounter = mementoScenarioData.GetFairyTalesData().GetBedtimeTalesOnboardingCounter();
    if (bedtimeTalesOnboardingCounter >= BEDTIME_TALES_ONBOARDING_MAX_COUNTER) {
        LOG_INFO(logger) << "Will not add bedtime tales onboarding nlg: memento counter value ("
            << bedtimeTalesOnboardingCounter
            << ") is equal or larger than max value ("
            << BEDTIME_TALES_ONBOARDING_MAX_COUNTER << ")";
        return false;
    }

    LOG_INFO(logger) << "Appending bedtime tales onboarding";
    return true;
}

void AddBedtimeTalesOnboardingAttention(NJson::TJsonValue& stateJson, bool isThinPlayer)
{
    Y_ENSURE(stateJson.Has("blocks"));
    NJson::TJsonValue bedtimeTalesAttention;
    bedtimeTalesAttention["type"] = "attention";
    bedtimeTalesAttention["attention_type"] = isThinPlayer ? "bedtime_tales_onboarding_thin_player" : "bedtime_tales_onboarding";
    stateJson["blocks"].AppendValue(std::move(bedtimeTalesAttention));
}

void AddBedtimeTalesOnboardingDirective(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    TResponseBodyBuilder& bodyBuilder)
{
    auto mementoDirective = bodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective();
    const TMusicScenarioMementoData& mementoScenarioData = ParseMementoData(applyRequest);
    const int bedtimeTalesOnboardingCounter = !applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES_FORCE_ONBOARDING) ?
        mementoScenarioData.GetFairyTalesData().GetBedtimeTalesOnboardingCounter() : 0;
    TMusicScenarioMementoData updatedScenarioData = mementoScenarioData;
    const int newBedtimeTalesOnboardingCounter = bedtimeTalesOnboardingCounter + 1;
    updatedScenarioData.MutableFairyTalesData()->SetBedtimeTalesOnboardingCounter(newBedtimeTalesOnboardingCounter);
    mementoDirective->MutableUserObjects()->MutableScenarioData()->PackFrom(updatedScenarioData);

    LOG_DEBUG(logger) << "Incremented bedtime tales onboarding nlg counter value: " << newBedtimeTalesOnboardingCounter;
}

} // namespace NAlice::NHollywood::NMusic
