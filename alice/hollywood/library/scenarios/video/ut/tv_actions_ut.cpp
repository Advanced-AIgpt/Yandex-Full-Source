#include <alice/library/json/json.h>
#include <alice/library/search_result_parser/video/parsers.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>
#include <alice/hollywood/library/scenarios/video/scene/voice_buttons.h>

namespace NAlice::NHollywood::NVideo {
    Y_UNIT_TEST_SUITE(Actions) {
        Y_UNIT_TEST(CheckResponse) {
            TFileInput inputData("fixtures/game_of_thrones.json");
            NJson::TJsonValue jsonResponse = JsonFromString(inputData.ReadAll());
            NAlice::TTvSearchResultData searchResultData = SearchResultParser::ParseJsonResponse(jsonResponse, TRTLogger::NullLogger());
            NHollywoodFw::NVideo::TVideoVoiceButtons::SetupTvActions(searchResultData);
            for (auto& carousel : *searchResultData.mutable_galleries()) {
                for (auto& item : *carousel.MutableBasicCarousel()->MutableItems()) {
                    if (item.HasVideoItem()) {
                        if (item.GetVideoItem().GetProviderItemId()) {
                            NAlice::NScenarios::TTvOpenDetailsScreenDirective dir;
                            item.GetOnClickAction().at(0).GetDirective().UnpackTo(&dir);
                            UNIT_ASSERT(item.GetVideoItem().GetProviderItemId() == dir.GetVhUuid());
                        } else {
                            NAlice::NScenarios::TVideoPlayDirective dir;
                            item.GetOnClickAction().at(0).GetDirective().UnpackTo(&dir);
                            UNIT_ASSERT(dir.item().GetEmbedUri() == item.GetVideoItem().GetEmbedUri());
                        }
                    }
                    if (item.HasPersonItem()) {
                        NAlice::NScenarios::TTvOpenPersonScreenDirective dir;
                        item.GetOnClickAction().at(0).GetDirective().UnpackTo(&dir);
                        UNIT_ASSERT(item.GetPersonItem().GetKpId() == dir.GetKpId());
                        UNIT_ASSERT(item.GetPersonItem().GetName() == dir.GetName());
                    }
                }
            }
        }
    }
}
