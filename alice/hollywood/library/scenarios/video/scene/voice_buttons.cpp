#include "voice_buttons.h"
#include <google/protobuf/any.pb.h>

void NAlice::NHollywoodFw::NVideo::TVideoVoiceButtons::SetupTvActions(NAlice::TTvSearchCarouselWrapper& carousel) {
    for (auto& item : *carousel.MutableBasicCarousel()->MutableItems()) {
        if (item.HasVideoItem()) {
            if (item.videoitem().GetProviderItemId()) {
                NAlice::TTvAction* newAction = item.AddOnClickAction();
                NAlice::NScenarios::TTvOpenDetailsScreenDirective openDirective;
                openDirective.SetVhUuid(item.GetVideoItem().GetProviderItemId());
                newAction->MutableDirective()->PackFrom(openDirective);
            } else {
                NAlice::TTvAction* newAction = item.AddOnClickAction();
                NAlice::NScenarios::TVideoPlayDirective playDirective;
                playDirective.SetUri(item.GetVideoItem().GetEmbedUri());
                newAction->MutableDirective()->PackFrom(playDirective);
            }
        }
        if (item.HasPersonItem()) {
            NAlice::TTvAction* newAction = item.AddOnClickAction();
            NAlice::NScenarios::TTvOpenPersonScreenDirective personScreenDirective;
            personScreenDirective.SetName((item.GetPersonItem().GetName()));
            personScreenDirective.SetKpId(item.GetPersonItem().GetKpId());
            newAction->MutableDirective()->PackFrom(personScreenDirective);
        }
    }
}

void NAlice::NHollywoodFw::NVideo::TVideoVoiceButtons::SetupTvActions(NAlice::TTvSearchResultData& searchResultData) {
    for (auto& carousel : *searchResultData.mutable_galleries()) {
        SetupTvActions(carousel);
    }
}