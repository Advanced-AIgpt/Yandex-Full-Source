#include "child_age_settings.h"
#include "semantic_frames.h"

#include <alice/hollywood/library/scenarios/music/proto/music_memento_scenario_data.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>
#include <alice/megamind/protos/common/app_type.pb.h>

#include <alice/protos/data/child_age.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString PUSH_TITLE = "Настройте сказки!";
const TString PUSH_TEXT = "Включу то, что подходит по возрасту";
const TString PUSH_URL = "https://yandex.ru/quasar/account/fairy-tales";
const TString PUSH_TAG = "alice_fairy_tales_settings";
const TString PUSH_POLICY = "bass-default-push";
const TVector<NAlice::EAppType> PUSH_APP_TYPES = { NAlice::EAppType::AT_SEARCH_APP };

const int FAIRY_TALES_CHILD_AGE_MAX_PROMO_COUNTER = 4;
const double FAIRY_TALES_CHILD_AGE_PROMO_PROBABILITY = 1. / 3;

void AddPushMessageDirective(TResponseBodyBuilder& bodyBuilder) {
    TPushDirectiveBuilder builder(PUSH_TITLE, PUSH_TEXT, PUSH_URL, PUSH_TAG);
    builder.SetPushId(PUSH_TAG);
    builder.SetAppTypes(PUSH_APP_TYPES);
    builder.SetThrottlePolicy(PUSH_POLICY);
    return builder.BuildTo(bodyBuilder);
}

} // namespace

bool ChildAgeIsSet(const NData::TChildAge& age) {
    return age.GetEpochSeconds() > 0;
}

bool ShouldAddChildAgePromo(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    IRng& rng)
{
    if (!applyRequest.Input().FindSemanticFrame(MUSIC_PLAY_FAIRYTALE_FRAME)) {
        LOG_INFO(logger) << "Will not add child age promo: no fairy tale semantic frame";
        return false;
    }
    if (!applyRequest.ClientInfo().IsSmartSpeaker()) {
        LOG_INFO(logger) << "Will not add child age promo: not a smart speaker";
        return false;
    }
    const auto& userConfigs = applyRequest.BaseRequestProto().GetMemento().GetUserConfigs();
    if (ChildAgeIsSet(userConfigs.GetChildAge())) {
        LOG_INFO(logger) << "Will not add child age promo: child age is already set";
        return false;
    }
    if (applyRequest.HasExpFlag(NExperiments::EXP_FAIRY_TALES_AGE_SELECTOR_FORCE_ALWAYS_SEND_PUSH)) {
        LOG_INFO(logger) << "Appending child age promo: counter overriden by experiment flag";
        return true;
    }
    const TMusicScenarioMementoData& mementoScenarioData = ParseMementoData(applyRequest);
    const int childAgePromoCounter = mementoScenarioData.GetFairyTalesData().GetChildAgePromoCounter();
    if (childAgePromoCounter >= FAIRY_TALES_CHILD_AGE_MAX_PROMO_COUNTER) {
        LOG_INFO(logger) << "Will not add child age promo: memento counter value ("
            << childAgePromoCounter
            << ") is equal or larger than max value ("
            << FAIRY_TALES_CHILD_AGE_MAX_PROMO_COUNTER << ")";
        return false;
    }
    if (rng.RandomDouble() < FAIRY_TALES_CHILD_AGE_PROMO_PROBABILITY) {
        LOG_INFO(logger) << "Appending child age promo: all conditions are met";
        return true;
    } else {
        LOG_INFO(logger) << "Will not add child age promo: disabled by random number generator value";
        return false;
    }
}

void AddChildAgePromoAttention(NJson::TJsonValue& stateJson) {
    if (stateJson.Has("blocks")) {
        NJson::TJsonValue childAgePromoAttention;
        childAgePromoAttention["type"] = "attention";
        childAgePromoAttention["attention_type"] = "show_child_age_promo";
        stateJson["blocks"].AppendValue(std::move(childAgePromoAttention));
    }
}

void AddChildAgePromoDirectives(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    TResponseBodyBuilder& bodyBuilder)
{
    AddPushMessageDirective(bodyBuilder);

    auto mementoDirective = bodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective();
    const TMusicScenarioMementoData& mementoScenarioData = ParseMementoData(applyRequest);
    const int childAgePromoCounter = mementoScenarioData.GetFairyTalesData().GetChildAgePromoCounter();
   TMusicScenarioMementoData updatedScenarioData = mementoScenarioData;
    const int newChildAgePromoCounter = childAgePromoCounter + 1;
    updatedScenarioData.MutableFairyTalesData()->SetChildAgePromoCounter(newChildAgePromoCounter);
    mementoDirective->MutableUserObjects()->MutableScenarioData()->PackFrom(updatedScenarioData);

    LOG_DEBUG(logger) << "Sending child age promo push message. Incremented counter value: " << newChildAgePromoCounter;
}

} // namespace NAlice::NHollywood::NMusic
