#include "greetings_scene.h"
#include "what_can_you_do_scene.h"

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/layout.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NOnboarding {
    Y_UNIT_TEST_SUITE(OnboardingRender) {

        Y_UNIT_TEST(GreetingsRender) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            TGreetingsRenderProto proto;
            TSkill skill1;
            skill1.SetTitle("First skill");
            skill1.SetActivation("Skill 1");
            skill1.SetActionId("action_0");
            TSkill skill2;
            skill2.SetTitle("Another skill");
            skill2.SetActivation("Skill 2");
            skill2.SetActionId("action_1");
            *proto.AddSkills() = skill1;
            *proto.AddSkills() = skill2;

            UNIT_ASSERT(testData >> TTestRender(&TGreetingsScene::Render, proto) >> testData);
            UNIT_ASSERT(!testData.IsIrrelevant());
            UNIT_ASSERT_EQUAL(testData.GetText(), "");
            UNIT_ASSERT_EQUAL(testData.GetVoice(), "");
            auto& body = testData.ResponseBody;
            auto& dirs = body.GetLayout().GetDirectives();
            UNIT_ASSERT_EQUAL(1u, dirs.size());
            //TODO: check div2
            UNIT_ASSERT(dirs[0].HasShowButtonsDirective());
            {
                const auto& directive = dirs[0].GetShowButtonsDirective();
                UNIT_ASSERT_EQUAL("cloud_ui", directive.GetScreenId());
                UNIT_ASSERT_EQUAL(2u, directive.ButtonsSize());
                const auto& buttons = directive.GetButtons();
                UNIT_ASSERT_EQUAL(skill1.GetTitle(), buttons[0].GetTitle());
                UNIT_ASSERT_EQUAL("action_0", buttons[0].GetActionId());
                UNIT_ASSERT_EQUAL(skill2.GetTitle(), buttons[1].GetTitle());
                UNIT_ASSERT_EQUAL("action_1", buttons[1].GetActionId());
                const auto& actions = body.GetFrameActions();
                UNIT_ASSERT_EQUAL(2u, actions.size());
                // Frame details are checked in it2 test
            }
        }

        Y_UNIT_TEST(WhatCanYouDoRenderMainScreen) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            TWhatCanYouDoRenderProto proto;
            proto.SetPhraseIndex(3);
            proto.SetIsTvPlugged(true);
            proto.SetScreenMode("main");

            UNIT_ASSERT(testData >> TTestRender(&TWhatCanYouDoScene::Render, proto) >> testData);
            UNIT_ASSERT(!testData.IsIrrelevant());
            UNIT_ASSERT(testData.ContainsText("Я могу включить вам русскую драму или американскую трагедию - словом, что попросите."));
            UNIT_ASSERT(testData.ContainsVoice("Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите."));

            const auto& actions = testData.ResponseBody.GetFrameActions();
            UNIT_ASSERT_EQUAL(actions.size(), 2u);
            UNIT_ASSERT(actions.contains("action_what_can_you_do_next"));
            UNIT_ASSERT(actions.contains("action_what_can_you_do_decline"));
        }

        Y_UNIT_TEST(WhatCanYouDoRenderSeasonGalleryScreen) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            TWhatCanYouDoRenderProto proto;
            proto.SetPhraseIndex(1);
            proto.SetIsTvPlugged(true);
            proto.SetScreenMode("season_gallery");

            UNIT_ASSERT(testData >> TTestRender(&TWhatCanYouDoScene::Render, proto) >> testData);
            UNIT_ASSERT(!testData.IsIrrelevant());
            UNIT_ASSERT(testData.ContainsText("Можно выбрать сезон сериала, сказав, например, «третий сезон». А также можно листать список серий, произнося «дальше» или «назад»."));
            UNIT_ASSERT(testData.ContainsVoice("Можно выбрать сезон сериала, сказав, например, «третий сезон». А также можно листать список серий, произнося «дальше» или «назад»."));

            const auto& actions = testData.ResponseBody.GetFrameActions();
            UNIT_ASSERT_EQUAL(actions.size(), 2u);
            UNIT_ASSERT(actions.contains("action_what_can_you_do_next"));
            UNIT_ASSERT(actions.contains("action_what_can_you_do_decline"));
        }

        Y_UNIT_TEST(WhatCanYouDoRenderEmptyScreen) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            TWhatCanYouDoRenderProto proto;
            proto.SetPhraseIndex(3);
            proto.SetIsTvPlugged(true);

            UNIT_ASSERT(testData >> TTestRender(&TWhatCanYouDoScene::Render, proto) >> testData);
            UNIT_ASSERT(!testData.IsIrrelevant());
            UNIT_ASSERT(testData.ContainsText("Я могу включить вам русскую драму или американскую трагедию - словом, что попросите."));
            UNIT_ASSERT(testData.ContainsVoice("Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите."));

            const auto& actions = testData.ResponseBody.GetFrameActions();
            UNIT_ASSERT_EQUAL(actions.size(), 2u);
            UNIT_ASSERT(actions.contains("action_what_can_you_do_next"));
            UNIT_ASSERT(actions.contains("action_what_can_you_do_decline"));
        }

        Y_UNIT_TEST(WhatCanYouDoRenderStopOnDecline) {
            TTestEnvironment testData(NProductScenarios::ONBOARDING, "ru-ru");
            testData.AddExp("hw_what_can_you_do_dont_stop_on_decline", "1");
            TWhatCanYouDoRenderProto proto;
            proto.SetPhraseIndex(3);
            proto.SetIsTvPlugged(true);
            proto.SetScreenMode("main");

            UNIT_ASSERT(testData >> TTestRender(&TWhatCanYouDoScene::Render, proto) >> testData);
            UNIT_ASSERT(!testData.IsIrrelevant());
            UNIT_ASSERT(testData.ContainsText("Я могу включить вам русскую драму или американскую трагедию - словом, что попросите."));
            UNIT_ASSERT(testData.ContainsVoice("Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите."));

            const auto& actions = testData.ResponseBody.GetFrameActions();
            UNIT_ASSERT_EQUAL(actions.size(), 3u);
            UNIT_ASSERT(actions.contains("action_what_can_you_do_next"));
            UNIT_ASSERT(actions.contains("action_what_can_you_do_decline"));
            UNIT_ASSERT(actions.contains("action_what_can_you_do_stop"));
        }
    }
} // namespace NAlice::NHollywoodFw::NOnboarding
