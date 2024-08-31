#include "current_screen_features.h"

#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <kernel/alice/device_state_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAliceDeviceState;
using namespace NVideoCommon;

namespace {

class TTestData {
public:
    TTestData() {
        NJson::TJsonValue testDataJson = JsonFromString(NResource::Find("device_state.json"));
        for (const auto& data : testDataJson.GetMap()) {
            deviceStateMap[FromString<EScreenId>(data.first)] = data.second;
        }
    }

    TDeviceState GetDeviceState(EScreenId screenId) {
        TDeviceState deviceState;
        JsonToProto(deviceStateMap[screenId], deviceState);
        return deviceState;
    }

private:
    TMap<EScreenId, NJson::TJsonValue> deviceStateMap;
};

TFactorStorage FillDeviceStateFactors(EScreenId screenId) {
    static TTestData testData;

    TFactorStorage storage = NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain());
    TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    FillCurrentScreen(testData.GetDeviceState(screenId), view);

    return storage;
}

void TestMainScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestGalleryScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestSeasonGalleryScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestDescriptionScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestVideoEntityCarouselScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestPaymentScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestMusicPlayerScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 1);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestVideoPlayerScreen(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 1);
}

void TestZero(EScreenId screenId) {
    TFactorStorage storage = FillDeviceStateFactors(screenId);
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MAIN], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_SEASON_GALLERY], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_DESCRIPTION], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_PAYMENT], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_MUSIC_PLAYER], 0);
    UNIT_ASSERT_EQUAL(view[FI_IS_SCREEN_VIDEO_PLAYER], 0);
}

void TestCase(EScreenId screenId) {
    switch(screenId) {
        case EScreenId::Bluetooth /* "bluetooth" */:
            [[fallthrough]];
        case EScreenId::RadioPlayer /* "radio_player" */:
            [[fallthrough]];
        case EScreenId::TvGallery /* "tv_gallery" */:
            TestZero(screenId);
            break;

        case EScreenId::MordoviaMain /* "mordovia_webview" */:
            [[fallthrough]];
        case EScreenId::Main /* "main" */:
            [[fallthrough]];
        case EScreenId::TvMain /* "tv_main" */:
            TestMainScreen(screenId);
            break;

        case EScreenId::Payment /* "payment" */:
            TestPaymentScreen(screenId);
            break;

        case EScreenId::VideoPlayer /* "video_player" */:
            TestVideoPlayerScreen(screenId);
            break;

        case EScreenId::MusicPlayer /* "music_player" */:
            TestMusicPlayerScreen(screenId);
            break;

        case EScreenId::ContentDetails /* "content_details" */:
            [[fallthrough]];
        case EScreenId::Description /* "description" */:
            [[fallthrough]];
        case EScreenId::WebViewVideoEntity /* "videoEntity" */:
            [[fallthrough]];
        case EScreenId::WebviewVideoEntityDescription /* "videoEntity/Description" */:
            TestDescriptionScreen(screenId);
            break;

        case EScreenId::WebviewVideoEntityRelated /* "videoEntity/RelatedCarousel" */:
            [[fallthrough]];
        case EScreenId::WebviewVideoEntityWithCarousel /* "videoEntity/Carousel" */:
            TestVideoEntityCarouselScreen(screenId);
            break;

        case EScreenId::Gallery /* "gallery" */:
            [[fallthrough]];
        case EScreenId::WebViewVideoSearchGallery /* "videoSearch" */:
            [[fallthrough]];
        case EScreenId::WebViewFilmsSearchGallery /* "filmsSearch" */:
            [[fallthrough]];
        case EScreenId::TvExpandedCollection /* "tv_expanded_collection" */:
            [[fallthrough]];
        case EScreenId::SearchResults /* "search_results" */:
            TestGalleryScreen(screenId);
            break;

        case EScreenId::SeasonGallery /* "season_gallery" */:
            [[fallthrough]];
        case EScreenId::WebviewVideoEntitySeasons /* "videoEntity/Seasons" */:
            TestSeasonGalleryScreen(screenId);
            break;

        default:
            Y_UNREACHABLE();
    }
}

} // namespace

Y_UNIT_TEST_SUITE(FillCurrentScreenFactors) {
    Y_UNIT_TEST(MainScreen) {
        TestCase(EScreenId::Main);
        TestCase(EScreenId::MordoviaMain);
        TestCase(EScreenId::TvMain);
    }
    Y_UNIT_TEST(GalleryScreen) {
        TestCase(EScreenId::Gallery);
        TestCase(EScreenId::WebViewFilmsSearchGallery);
        TestCase(EScreenId::WebViewFilmsSearchGallery);
        TestCase(EScreenId::WebviewVideoEntityRelated);
        TestCase(EScreenId::SearchResults);
        TestCase(EScreenId::TvExpandedCollection);
    }
    Y_UNIT_TEST(SeasonGalleryScreen) {
        TestCase(EScreenId::SeasonGallery);
        TestCase(EScreenId::WebviewVideoEntitySeasons);
    }
    Y_UNIT_TEST(DescriptionScreen) {
        TestCase(EScreenId::ContentDetails);
        TestCase(EScreenId::Description);
        TestCase(EScreenId::WebViewVideoEntity);
        TestCase(EScreenId::WebviewVideoEntityDescription);
    }
    Y_UNIT_TEST(WebViewVideoEntityCarouselScreen) {
        TestCase(EScreenId::WebviewVideoEntityWithCarousel);
    }
    Y_UNIT_TEST(PaymentScreen) {
        TestCase(EScreenId::Payment);
    }
    Y_UNIT_TEST(MusicPlayerScreen) {
        TestCase(EScreenId::MusicPlayer);
    }
    Y_UNIT_TEST(TestVideoPlayerScreen) {
        TestCase(EScreenId::VideoPlayer);
    }
    Y_UNIT_TEST(TestOtherScreens) {
        TestCase(EScreenId::Bluetooth);
        TestCase(EScreenId::TvGallery);
        TestCase(EScreenId::RadioPlayer);
    }

} // namespace
