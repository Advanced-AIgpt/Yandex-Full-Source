#include "item_selector.h"

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logadapter.h>
#include <alice/library/unittest/mock_logadapter.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NBASS::NVideoCommon;

namespace {

constexpr TStringBuf EMPTY_REQUEST = TStringBuf(R"({})");

constexpr TStringBuf REQUEST_WITH_MOVIES = TStringBuf(R"(
{
    "video": {
        "current_screen": "gallery",
        "screen_state": {
            "items": [
                {"name": "какой-то левый фильм"},
                {"name": "игра"},
                {"name": "игра престолов"},
                {"name": "зеркало"},
                {"name": "черное зеркало"}
            ]
        }
    }
})");

constexpr TStringBuf REQUEST_WITH_INVISIBLE_MOVIES = TStringBuf(R"(
{
    "video": {
        "current_screen": "gallery",
        "screen_state": {
            "items": [
                {"name": "какой-то левый фильм"},
                {"name": "игра"},
                {"name": "игра престолов"},
                {"name": "зеркало"},
                {"name": "черное зеркало"}
            ],
            "visible_items": [2, 3]
        }
    }
})");

constexpr TStringBuf REQUEST_WITH_WEBVIEW_SCREEN = TStringBuf(R"(
{
    "video": {
        "current_screen": "mordovia_webview",
        "view_state": {
            "currentScreen": "videoEntity/Seasons",
            "sections": [{
                "items": [
                    {"title": "какой-то левый фильм", "active": true, "number": 1},
                    {"title": "игра", "active": true, "number": 2},
                    {"title": "игра престолов", "active": false, "number": 3},
                    {"title": "зеркало", "active": false, "number": 4},
                    {"title": "черное зеркало", "active": false, "number": 5}
                ]
            }]
        }
    }
}
)");

Y_UNIT_TEST_SUITE(TestScenario) {
    Y_UNIT_TEST(EmptySelection) {
        TDeviceState deviceState;
        JsonToProto(JsonFromString(EMPTY_REQUEST), deviceState);

        TString videoText = "игра";
        TItemSelectionResult result = SelectVideoFromGallery(deviceState, videoText, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, -1);
        UNIT_ASSERT_EQUAL(result.Confidence, -1.0); // no items to select from -> default score
    }

    Y_UNIT_TEST(BasicSelectionExactMatch) {
        TDeviceState deviceState;
        JsonToProto(JsonFromString(REQUEST_WITH_MOVIES), deviceState);

        TString game = "игра";
        TString blackMirror = "черное зеркало";
        TItemSelectionResult result;

        result = SelectVideoFromGallery(deviceState, game, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, 2);
        UNIT_ASSERT_EQUAL(result.Confidence, 1.0); // exact match -> the maximal score

        result = SelectVideoFromGallery(deviceState, blackMirror, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, 5);
        UNIT_ASSERT_EQUAL(result.Confidence, 1.0); // exact match -> the maximal score
    }

    Y_UNIT_TEST(BasicSelectionPartialMatch) {
        TDeviceState deviceState;
        JsonToProto(JsonFromString(REQUEST_WITH_INVISIBLE_MOVIES), deviceState);

        TString game = "игра";
        TString blackMirror = "черное зеркало";
        TItemSelectionResult result;

        result = SelectVideoFromGallery(deviceState, game, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, 3);
        UNIT_ASSERT(result.Confidence > 0.2); // partial match (query < result) -> score is intermediate
        UNIT_ASSERT(result.Confidence < 0.8);

        result = SelectVideoFromGallery(deviceState, blackMirror, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, 4);
        UNIT_ASSERT(result.Confidence > 0.2); // partial match (query > result) -> score is intermediate
        UNIT_ASSERT(result.Confidence < 0.8);
    }

    Y_UNIT_TEST(SelecttionFromWebviewScreen) {
        TDeviceState deviceState;
        JsonToProto(JsonFromString(REQUEST_WITH_WEBVIEW_SCREEN), deviceState);

        TString game = "игра";
        TString blackMirror = "черное зеркало";
        TItemSelectionResult result;

        result = SelectVideoFromGallery(deviceState, game, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, 2);
        UNIT_ASSERT_EQUAL(result.Confidence, 1.0); // exact match -> the maximal score

        result = SelectVideoFromGallery(deviceState, blackMirror, TMockLogAdapter{});
        UNIT_ASSERT_EQUAL(result.Index, -1);
        UNIT_ASSERT_EQUAL(result.Confidence, -1.0); // no match among visible items -> default score
    }
}

} // namespace
