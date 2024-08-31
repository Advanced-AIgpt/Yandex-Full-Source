#include "data_sources.h"

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_global_context.h>
#include <alice/megamind/library/testing/mock_responses.h>

#include <alice/megamind/protos/common/tandem_state.pb.h>
#include <alice/megamind/protos/scenarios/begemot.pb.h>
#include <alice/megamind/protos/scenarios/notification_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/library/testing/apphost_helpers.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

using namespace ::testing;

namespace {
constexpr TStringBuf NOTIFICATION_STATE_RESOURCE_KEY = "notification_state.json";
constexpr TStringBuf WIZARD_RESPONSE_RESOURCE_KEY = "wizard_response.json";
constexpr TStringBuf VINS_RULES_RESOURCE_KEY = "vins_rules.json";
constexpr TStringBuf VIDEO_VIEW_STATE_RESOURCE_KEY = "video_view_state.json";
constexpr TStringBuf AUXILIARY_CONFIG_RESOURCE_KEY = "auxiliary_config.json";
constexpr TStringBuf ALICE4BUSINESS_CONFIG_RESOURCE_KEY = "alice4business_config.json";
constexpr TStringBuf BEGEMOT_FIXLIST_RESOURCE_KEY = "begemot_fixlist.json";
constexpr TStringBuf VIDEO_CURRENTLY_PLAYING_RESOURCE_KEY = "video_currently_playing.json";

NJson::TJsonValue GetJsonResource(const TStringBuf key) {
    return JsonFromString(NResource::Find(key));
}

TWizardResponse ReadWizardResponse() {
    const auto begemotResponseJson = GetJsonResource(WIZARD_RESPONSE_RESOURCE_KEY);
    auto begemotResponse = NAlice::JsonToProto<NBg::NProto::TAlicePolyglotMergeResponseResult>(begemotResponseJson);
    return TWizardResponse(std::move(begemotResponse));
}

} // namespace

Y_UNIT_TEST_SUITE(TestDataSources) {
    Y_UNIT_TEST(TestVinsDataSource) {
        NiceMock<TMockResponses> responses{};
        const auto wizardResponse = ReadWizardResponse();
        EXPECT_CALL(responses, WizardResponse(_)).WillOnce(ReturnRef(wizardResponse));
        const auto rawEntitySearchResponse = JsonFromString(TStringBuf(R"({"key": "value"})"));
        TEntitySearchResponse entitySearchResponse{rawEntitySearchResponse};
        EXPECT_CALL(responses, EntitySearchResponse(_)).WillOnce(ReturnRef(entitySearchResponse));

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{&responses,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        {
            const auto& actual = dataSources.GetDataSource(EDataSourceType::VINS_WIZARD_RULES);
            expected.MutableVinsWizardRules()->SetRawJson(JsonToString(GetJsonResource(VINS_RULES_RESOURCE_KEY)));
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
        }
        {
            const auto& actual = dataSources.GetDataSource(EDataSourceType::ENTITY_SEARCH);
            expected.MutableEntitySearch()->SetRawJson(JsonToString(rawEntitySearchResponse));
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
            UNIT_ASSERT_MESSAGES_EQUAL(actual, dataSources.GetDataSource(EDataSourceType::ENTITY_SEARCH));
        }
    }

    Y_UNIT_TEST(TestBegemotFixlistDataSource) {
        NiceMock<TMockResponses> responses{};
        const auto wizardResponse = ReadWizardResponse();
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{&responses,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState = */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        {
            auto actual = dataSources.GetDataSource(EDataSourceType::BEGEMOT_FIXLIST_RESULT);
            const auto begemotFixlistJson = GetJsonResource(BEGEMOT_FIXLIST_RESOURCE_KEY);
            JsonToProto(begemotFixlistJson, *expected.MutableBegemotFixlistResult());
            const auto getSortingKey = [](const auto& match) { return match.GetKey(); };
            SortBy(*actual.MutableBegemotFixlistResult()->MutableMatches(), getSortingKey);
            SortBy(*expected.MutableBegemotFixlistResult()->MutableMatches(),getSortingKey);
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
        }
    }

    Y_UNIT_TEST(TestVideoViewStateDataSource) {
        google::protobuf::Struct videoViewState;
        const auto viewStateJson = GetJsonResource(VIDEO_VIEW_STATE_RESOURCE_KEY);
        JsonToProto(viewStateJson, videoViewState);


        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 &videoViewState,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        {
            const auto& actual = dataSources.GetDataSource(EDataSourceType::VIDEO_VIEW_STATE);
            JsonToProto(viewStateJson, *expected.MutableVideoViewState()->MutableViewState());
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
        }
    }

    Y_UNIT_TEST(TestNotificationStateDataSource) {
        TNotificationState notificationState;
        const auto notificationStateJson = GetJsonResource(NOTIFICATION_STATE_RESOURCE_KEY);
        JsonToProto(notificationStateJson, notificationState);


        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 &notificationState,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        {
            const auto& actual = dataSources.GetDataSource(EDataSourceType::NOTIFICATION_STATE);
            JsonToProto(notificationStateJson, *expected.MutableNotificationState());
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
        }
    }

    Y_UNIT_TEST(TestEmptyAlice4BusinessDataSource) {
        NQuasarAuxiliaryConfig::TAuxiliaryConfig auxiliaryconfig;

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ &auxiliaryconfig,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        const auto& actual = dataSources.GetDataSource(EDataSourceType::ALICE4BUSINESS_DEVICE);
        UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TestEmptyAlice4BusinessDataSourceFilled) {
        NQuasarAuxiliaryConfig::TAuxiliaryConfig auxiliaryconfig;
        const auto auxiliaryconfigJson = GetJsonResource(AUXILIARY_CONFIG_RESOURCE_KEY);
        JsonToProto(auxiliaryconfigJson, auxiliaryconfig);

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ &auxiliaryconfig,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        const auto& actual = dataSources.GetDataSource(EDataSourceType::ALICE4BUSINESS_DEVICE);
        const auto alice4businessJson = GetJsonResource(ALICE4BUSINESS_CONFIG_RESOURCE_KEY);
        JsonToProto(alice4businessJson, *expected.MutableAlice4BusinessConfig());
        UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TestDeviceStateNavigatorDataSourceWithEmptyNavigator) {
        TDeviceState deviceState{};
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};

        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 deviceState,
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::DEVICE_STATE_NAVIGATOR);
        UNIT_ASSERT_MESSAGES_EQUAL(actual, NScenarios::TDataSource::default_instance());
    }

    Y_UNIT_TEST(TestDeviceStateNavigatorDataSourceWithFilledNavigator) {
        TDeviceState deviceState{};
        deviceState.MutableNavigator()->AddStates("states");
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};

        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 deviceState,
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::DEVICE_STATE_NAVIGATOR);
        NScenarios::TDataSource expected{};
        expected.MutableDeviceStateNavigator()->CopyFrom(deviceState.GetNavigator());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestEmptyDeviceStateDataSource) {
        TDeviceState deviceState{};
        deviceState.MutableNavigator()->AddStates("states");
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};

        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 deviceState,
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::EMPTY_DEVICE_STATE);
        UNIT_ASSERT_MESSAGES_EQUAL(actual, NScenarios::TDataSource::default_instance());
    }

    Y_UNIT_TEST(TestSaasSkillDiscoveryDataSource) {
        NScenarios::TSkillDiscoverySaasCandidates saasResult;
        saasResult.AddSaasCandidate()->SetSkillId("skill_id");
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};

        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 &saasResult,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto& actual = dataSources.GetDataSource(EDataSourceType::SKILL_DISCOVERY_GC);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetSkillDiscoveryGcSaasCandidates(), saasResult);
    }

    Y_UNIT_TEST(TestIoTUserInfoDataSource) {
        TIoTUserInfo ioTUserInfo;
        {
            auto& color = *ioTUserInfo.AddColors();
            color.SetId("raspberry");
            color.SetName("Малиновый");
        }

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 &ioTUserInfo,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::IOT_USER_INFO);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetIoTUserInfo(), ioTUserInfo);
    }

    Y_UNIT_TEST(TestAppInfoDataSource) {
        NScenarios::TAppInfo AppInfo;
        AppInfo.SetValue("eyJicm93c2VyTmFtZSI6Ik90aGVyQXBwbGljYXRpb25zIiwiZGV2aWNlVHlwZSI6InN0YXRpb24iLCJtb2JpbGVQbGF0"
                         "Zm9ybSI6ImFuZHJvaWQifQ==");

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo */ nullptr,
                                 &AppInfo,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::APP_INFO);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetAppInfo(), AppInfo);
    }

    Y_UNIT_TEST(TestRawPersonalData) {

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        {
            TDataSources dataSources{/* responses= */ nullptr,
                                     /* userLocation= */ nullptr,
                                     /* dialogHistory= */ nullptr,
                                     /* actions = */ nullptr,
                                     /* layout = */ nullptr,
                                     /* smartHomeInfo= */ nullptr,
                                     /* videoViewState= */ nullptr,
                                     /* notificationState= */ nullptr,
                                     /* skillDiscoverySaasCandidates= */ nullptr,
                                     /* auxiliaryConfig= */ nullptr,
                                     TRTLogger::StderrLogger(),
                                     /* deviceState */ {},
                                     /* ioTUserInfo */ nullptr,
                                     /* appInfo= */ nullptr,
                                     ahCtx.ItemProxyAdapter(),
                                     /* rawPersonalData= */ {},
                                     /* videoCurrentlyPlaying= */ nullptr,
                                     /* contactsList= */ nullptr,
                                     /* environmentState= */ nullptr,
                                     /* tandemEnvironmentState= */ nullptr,
                                     /* webSearchQuery */ "",
                                     /* whisperInfo= */ Nothing(),
                                     /* guestData= */ Nothing(),
                                     /* guestOptions= */ Nothing()};
            const auto actual = dataSources.GetDataSource(EDataSourceType::RAW_PERSONAL_DATA);
            UNIT_ASSERT(!actual.HasRawPersonalData());
        }
        {
            const TString data = "someDataHere";
            TDataSources dataSources{/* responses= */ nullptr,
                                     /* userLocation= */ nullptr,
                                     /* dialogHistory= */ nullptr,
                                     /* actions = */ nullptr,
                                     /* layout = */ nullptr,
                                     /* smartHomeInfo= */ nullptr,
                                     /* videoViewState= */ nullptr,
                                     /* notificationState= */ nullptr,
                                     /* skillDiscoverySaasCandidates= */ nullptr,
                                     /* auxiliaryConfig= */ nullptr,
                                     TRTLogger::StderrLogger(),
                                     /* deviceState */ {},
                                     /* ioTUserInfo */ nullptr,
                                     /* appInfo= */ nullptr,
                                     ahCtx.ItemProxyAdapter(),
                                     /* rawPersonalData= */ data,
                                     /* videoCurrentlyPlaying= */ nullptr,
                                     /* contactsList= */ nullptr,
                                     /* environmentState= */ nullptr,
                                     /* tandemEnvironmentState= */ nullptr,
                                     /* webSearchQuery */ "",
                                     /* whisperInfo= */ Nothing(),
                                     /* guestData= */ Nothing(),
                                     /* guestOptions= */ Nothing()};
            const auto actual = dataSources.GetDataSource(EDataSourceType::RAW_PERSONAL_DATA);
            UNIT_ASSERT(actual.HasRawPersonalData());
            UNIT_ASSERT_VALUES_EQUAL(actual.GetRawPersonalData(), data);
        }
    }

    Y_UNIT_TEST(TestVideoCurrentlyPlayingSource) {
        NAlice::TDeviceState_TVideo_TCurrentlyPlaying videoCurrentlyPlaying;
        const auto videoCurrentlyPlayingJson = GetJsonResource(VIDEO_CURRENTLY_PLAYING_RESOURCE_KEY);
        JsonToProto(videoCurrentlyPlayingJson, videoCurrentlyPlaying);

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 &videoCurrentlyPlaying,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        NScenarios::TDataSource expected{};
        {
            const auto& actual = dataSources.GetDataSource(EDataSourceType::VIDEO_CURRENTLY_PLAYING);
            JsonToProto(videoCurrentlyPlayingJson,
                        *expected.MutableVideoCurrentlyPlaying()->MutableCurrentlyPlaying());
            UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
        }
    }

    Y_UNIT_TEST(TestContactsListDataSource) {
        NAlice::NData::TContactsList contactsList;
        {
            contactsList.SetIsKnownUuid(true);

            auto& contact = *contactsList.AddContacts();
            contact.SetId(43);
            contact.SetLookupKey("abc");
            contact.SetAccountName("test@gmail.com");
            contact.SetFirstName("Test");
            contact.SetContactId(123);
            contact.SetAccountType("com.google");
            contact.SetDisplayName("Test");

            auto& phone = *contactsList.AddPhones();
            phone.SetId(44);
            phone.SetLookupKey("abc");
            phone.SetAccountType("com.google");
            phone.SetPhone("+79121234567");
            phone.SetType("mobile");
        }

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 &contactsList,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::CONTACTS_LIST);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetContactsList(), contactsList);
    }

    Y_UNIT_TEST(TestTandemEnvironmentStateDataSource) {
        TTandemEnvironmentState tandemEnvironmentState;
        {
            auto& device = *tandemEnvironmentState.AddDevices();
            device.MutableApplication()->SetAppId("ru.yandex.quasar.app");
            device.MutableTandemDeviceState()->MutableTandemState()->SetConnected(true);
        }

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 &tandemEnvironmentState,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::TANDEM_ENVIRONMENT_STATE);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetTandemEnvironmentState(), tandemEnvironmentState);
    }

    Y_UNIT_TEST(TestEnvironmentStateDataSource) {
        TEnvironmentState environmentState;
        {
            auto& device = *environmentState.AddDevices();
            device.MutableApplication()->SetAppId("ru.yandex.quasar.app");
        }

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 &environmentState,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::ENVIRONMENT_STATE);
        UNIT_ASSERT_MESSAGES_EQUAL(actual.GetEnvironmentState(), environmentState);
    }

    Y_UNIT_TEST(TestWebSearchQueryDataSource) {
        const TString query{"сколько весит слон"};

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 query,
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::WEB_SEARCH_REQUEST_META);
        UNIT_ASSERT_EQUAL(actual.GetWebSearchRequestMeta().GetQuery(), query);
    }

    Y_UNIT_TEST(TestWhisperInfoDataSource) {
        NMementoApi::TTtsWhisperConfig whisperConfig{};
        whisperConfig.SetEnabled(true);

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        const TMaybe<TRequest::TWhisperInfo> whisperInfo =
            TRequest::TWhisperInfo{/* isVoiceInput= */ true, /* lastWhisperTimeMs= */ 10,
                                   /* serverTimeMs= */ 10,   /* whisperTtlMs= */ 10,
                                   /* isAsrWhisper= */ true, whisperConfig};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ whisperInfo,
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::WHISPER_INFO);
        UNIT_ASSERT(actual.GetWhisperInfo().GetIsAsrWhisper());
        UNIT_ASSERT(actual.GetWhisperInfo().GetIsWhisperResponseAvailable());
    }

    Y_UNIT_TEST(TestGuestDataDataSource) {

        TMaybe<NAlice::TGuestData> guestData = NAlice::TGuestData{};
        guestData->SetRawPersonalData("test_raw_personal_data");

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ guestData,
                                 /* guestOptions= */ Nothing()};
        const auto actual = dataSources.GetDataSource(EDataSourceType::GUEST_DATA);
        UNIT_ASSERT_STRINGS_EQUAL(actual.GetGuestData().GetRawPersonalData(), "test_raw_personal_data");
    }

    Y_UNIT_TEST(TestGuestOptionsDataSource) {

        TMaybe<NAlice::TGuestOptions> guestOptions = NAlice::TGuestOptions{};
        guestOptions->SetPersId("kiy");

        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources{/* responses= */ nullptr,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr,
                                 TRTLogger::StderrLogger(),
                                 /* deviceState */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ guestOptions};
        const auto actual = dataSources.GetDataSource(EDataSourceType::GUEST_OPTIONS);
        UNIT_ASSERT_STRINGS_EQUAL(actual.GetGuestOptions().GetPersId(), "kiy");
    }
}

} // namespace NAlice::NMegamind
