#include "saved_progress_helper.h"

#include <alice/hollywood/library/scenarios/music/proto/music_memento_scenario_data.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/music/defs.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

namespace{

const int CAN_START_FROM_THE_BEGINNING_MAX_COUNTER = 4;

}

void TryAddCanStartFromTheBeginningAttention(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    const TMusicArguments& applyArgs,
    TResponseBodyBuilder& bodyBuilder,
    NJson::TJsonValue& stateJson)
{
    auto mementoDirective = bodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective();
    const TMusicScenarioMementoData& mementoScenarioData = ParseMementoData(applyRequest);
    const int counter = mementoScenarioData.GetSavedProgressData().GetCanStartFromTheBeginningCounter();
    if (counter < CAN_START_FROM_THE_BEGINNING_MAX_COUNTER &&
        (applyArgs.GetMusicSearchResult().GetSubtype() == "fairy-tale" || applyArgs.GetMusicSearchResult().GetSubtype() == "audiobook" ||
         applyArgs.GetMusicSearchResult().GetSubtype() == "podcast"))
    {
        LOG_INFO(logger) << "Adding attention that User can start content from the beginning, incrementing counter";
        TMusicScenarioMementoData updatedScenarioData = mementoScenarioData;
        const int newCounter = counter + 1;
        updatedScenarioData.MutableSavedProgressData()->SetCanStartFromTheBeginningCounter(newCounter);
        mementoDirective->MutableUserObjects()->MutableScenarioData()->PackFrom(updatedScenarioData);
        AddAttentionToJsonState(stateJson, TString{NAlice::NMusic::ATTENTION_CAN_START_FROM_THE_BEGINNING});
    } else {
        LOG_INFO(logger) << "No attention about starting content from the beginning has been added";
    }
}

} // namespace NAlice::NHollywood::NMusic
