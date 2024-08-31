#include "onboarding.h"

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NOnboarding {
    namespace {
        constexpr const TStringBuf GREETINGS_FRAME_NAME = "alice.onboarding.get_greetings";
        constexpr const TStringBuf WHAT_CAN_YOU_DO_FRAME_NAME = "alice.onboarding.what_can_you_do";

        void AddGreetingsFrame(TTestEnvironment& testData) {
            if (auto* semanticFrame = testData.RunRequest.MutableInput()->AddSemanticFrames()) {
                semanticFrame->SetName(TString{GREETINGS_FRAME_NAME});
                TOnboardingGetGreetingsSemanticFrame frame;
                *semanticFrame->MutableTypedSemanticFrame()->MutableOnboardingGetGreetingsSemanticFrame() = std::move(frame);
            }
        }

        void AddWhatCanYouDoFrame(TTestEnvironment& testData) {
            if (auto* semanticFrame = testData.RunRequest.MutableInput()->AddSemanticFrames()) {
                semanticFrame->SetName(TString{WHAT_CAN_YOU_DO_FRAME_NAME});
                TOnboardingWhatCanYouDoSemanticFrame frame;
                frame.MutablePhraseIndex()->SetUInt32Value(1);
                *semanticFrame->MutableTypedSemanticFrame()->MutableOnboardingWhatCanYouDoSemanticFrame() = std::move(frame);
            }
        }

        void EnableGreetings(TTestEnvironment& testData) {
            testData.AddExp("hw_onboarding_enable_greetings", "1");
        }

        void DisableWhatCanYouDo(TTestEnvironment& testData) {
            testData.AddExp("hw_onboarding_disable_what_can_you_do", "1");
        }
    }

    Y_UNIT_TEST_SUITE(OnboardingDispatch) {

        Y_UNIT_TEST(OnboardingDispatchGreetingsNoCloudUiFilling) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.IsIrrelevant());
        }

        Y_UNIT_TEST(OnboardingDispatchGreetingsDisabled) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddGreetingsFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetSupportsCloudUiFilling(true);

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.IsIrrelevant());
        }

        Y_UNIT_TEST(OnboardingDispatchGreetings) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetSupportsCloudUiFilling(true);

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "greetings");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), GREETINGS_FRAME_NAME);
            // TODO: check args if/when available
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoUnsupportedDevice) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.searchplugin");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.IsIrrelevant());
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoDisabled) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            DisableWhatCanYouDo(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.IsIrrelevant());
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoOkMainScreenTv) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->SetIsTvPluggedIn(true);
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("main");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoOkMordoviaScreen) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->SetIsTvPluggedIn(true);
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("mordovia_webview");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoOkMainScreenWithoutTv) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("music_player");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoOkGalleryScreen) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->SetIsTvPluggedIn(true);
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("gallery");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchWhatCanYouDoOkPlayer) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableMusic()->MutablePlayer()->SetPause(false);
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableMusic()->MutableCurrentlyPlaying()->SetTrackId("track_1");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesIrrelevant) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            EnableGreetings(testData);

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);
            UNIT_ASSERT(testData.IsIrrelevant());
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesOk) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetSupportsCloudUiFilling(true);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("main");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesUnsupportedGreetings) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");
            testData.RunRequest.MutableBaseRequest()->MutableDeviceState()->MutableVideo()->SetCurrentScreen("main");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesUnsupportedWhatCanYouDo) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.searchplugin");
            testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetSupportsCloudUiFilling(true);

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "greetings");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), GREETINGS_FRAME_NAME);
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesDisabledGreetings) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            testData.RunRequest.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar.app");

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "what_can_you_do");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), WHAT_CAN_YOU_DO_FRAME_NAME);
            const auto& resultArgs = testData.GetSelectedSceneArguments();
            TWhatCanYouDoSceneArgs sceneArguments;
            UNIT_ASSERT(resultArgs.UnpackTo(&sceneArguments));
            UNIT_ASSERT(sceneArguments.GetPhraseIndex() == 1);
        }

        Y_UNIT_TEST(OnboardingDispatchBothFramesDisabledWhatCanYouDo) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            AddWhatCanYouDoFrame(testData);
            AddGreetingsFrame(testData);
            EnableGreetings(testData);
            DisableWhatCanYouDo(testData);
            testData.RunRequest.MutableBaseRequest()->MutableInterfaces()->SetSupportsCloudUiFilling(true);

            UNIT_ASSERT(testData >> TTestDispatch(&TOnboardingScenario::Dispatch) >> testData);

            UNIT_ASSERT_EQUAL(testData.GetSelectedSceneName(), "greetings");
            UNIT_ASSERT_EQUAL(testData.GetSelectedIntent(), GREETINGS_FRAME_NAME);
        }
    }
} // namespace NAlice::NHollywoodFw::NOnboarding
