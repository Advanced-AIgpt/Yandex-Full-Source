#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/ut/nlg/register.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

const TStringBuf SCENARIO_NAME = "my_test_scenario";
const TStringBuf PSN_FOR_SCENARIO = "override_my_test_scenario";
const TStringBuf PSN_FOR_SCENE = "override_my_test_scene";
const TStringBuf PSN_FOR_CUSTOM = "override_custom_psn";
const TString FRAME1_INTENT_NAME = "frame1_intent_name";
const TString FRAME2_CUSTOM_NAME = "frame2_custom_name";

enum class ETestMode {
    DefaultPsn,
    StandardPsn,
    ScenePsn_IntentDispatch,
    ScenePsn_IntentMain,
    ScenePsn_IntentRender,
    CustomPsnDispatch,
    CustomPsnMain,
    CustomPsnRender,
    Actions
};

class TTestScene1: public TScene<TProtoUtScene1> {
public:
    TTestScene1(const TScenario* owner)
        : TScene(owner, "test_scene1")
    {
        RegisterRenderer(&TTestScene1::Render);
    }
    TRetMain Main(const TProtoUtScene1& sceneArgs, const TRunRequest& request, TStorage&, const TSource&) const override {
        ETestMode mode = static_cast<ETestMode>(sceneArgs.GetValue());
        if (mode == ETestMode::CustomPsnMain) {
            request.AI().OverrideProductScenarioName(PSN_FOR_CUSTOM);
        }
        TProtoUtRenderer1 renderArgs;
        renderArgs.SetValue(sceneArgs.GetValue());
        return TReturnValueRender(&TTestScene1::Render, renderArgs);
    }
    TRetResponse Render(const TProtoUtRenderer1& renderArgs, TRender& render) const {
        ETestMode mode = static_cast<ETestMode>(renderArgs.GetValue());
        if (mode == ETestMode::CustomPsnRender) {
            render.GetRequest().AI().OverrideProductScenarioName(PSN_FOR_CUSTOM);
        }
        return TReturnValueSuccess();
    }
};

class TTestScene2: public TScene<TProtoUtScene2> {
public:
    TTestScene2(const TScenario* owner)
        : TScene(owner, "test_scene2")
    {
        SetProductScenarioName(PSN_FOR_SCENE);
        RegisterRenderer(&TTestScene2::Render);
    }
    TRetMain Main(const TProtoUtScene2& sceneArgs, const TRunRequest& request, TStorage&, const TSource&) const override {
        ETestMode mode = static_cast<ETestMode>(sceneArgs.GetValue());
        if (mode == ETestMode::ScenePsn_IntentMain) {
            request.AI().OverrideIntent(FRAME2_CUSTOM_NAME);
        }
        TProtoUtRenderer2 renderArgs;
        renderArgs.SetValue(sceneArgs.GetValue());
        return TReturnValueRender(&TTestScene2::Render, renderArgs);
    }
    TRetResponse Render(const TProtoUtRenderer2& renderArgs, TRender& render) const {
        ETestMode mode = static_cast<ETestMode>(renderArgs.GetValue());
        if (mode == ETestMode::ScenePsn_IntentRender) {
            render.GetRequest().AI().OverrideIntent(FRAME2_CUSTOM_NAME);
            render.GetRequest().AI().OverrideResultSemanticFrame(FRAME1_INTENT_NAME);
        }
        return TReturnValueSuccess();
    }
};

class TTestScenario: public TScenario {
public:
    TTestScenario(ETestMode mode)
        : TScenario(SCENARIO_NAME)
        , Mode_(mode)
    {
        Register(&TTestScenario::Dispatch);
        RegisterScene<TTestScene1>([this]() {
            RegisterSceneFn(&TTestScene1::Main);
        });
        RegisterScene<TTestScene2>([this]() {
            RegisterSceneFn(&TTestScene2::Main);
        });
        // Set custom PSN
        if (mode == ETestMode::StandardPsn) {
            SetProductScenarioName(PSN_FOR_SCENARIO);
        }
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
    }

private:
    TRetScene Dispatch(const TRunRequest& request, const TStorage&, const TSource&) const {
        if (Mode_ == ETestMode::ScenePsn_IntentDispatch) {
            request.AI().OverrideIntent(FRAME2_CUSTOM_NAME);
        }
        if (Mode_ == ETestMode::ScenePsn_IntentDispatch ||
            Mode_ == ETestMode::ScenePsn_IntentMain ||
            Mode_ == ETestMode::ScenePsn_IntentRender) {
            TProtoUtScene2 sceneArgs;
            sceneArgs.SetValue((int)Mode_);
            return TReturnValueScene<TTestScene2>(sceneArgs, FRAME1_INTENT_NAME); // Note: FRAME1_INTENT_NAME is always owerwritten
        }
        if (Mode_ == ETestMode::CustomPsnDispatch) {
            request.AI().OverrideProductScenarioName(PSN_FOR_CUSTOM);
        }
        if (Mode_ == ETestMode::Actions) {
            NScenarios::TAnalyticsInfo::TAction action;
            action.SetId("Action");
            action.SetName("ActionName");
            request.AI().AddAction(std::move(action));
        }
        TProtoUtScene1 sceneArgs;
        sceneArgs.SetValue((int)Mode_);
        return TReturnValueScene<TTestScene1>(sceneArgs, FRAME1_INTENT_NAME);
    }

private:
    ETestMode Mode_;
};

} // anonimous namespace

Y_UNIT_TEST_SUITE(HollywoodFrameworkAITest) {
    Y_UNIT_TEST(TestDefaultPsn) {
        TTestScenario testScenario(ETestMode::DefaultPsn);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        // Validate that PSN is default scenario name
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetProductScenarioName(), SCENARIO_NAME);
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetIntent(), FRAME1_INTENT_NAME);
    } // Y_UNIT_TEST(TestDefaultPsn)
    Y_UNIT_TEST(TestStandardPsn) {
        TTestScenario testScenario(ETestMode::StandardPsn);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        // Validate that PSN is default scenario name
        UNIT_ASSERT(testEnv.ResponseBody.GetAnalyticsInfo().GetActions().size() == 0);
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetProductScenarioName(), PSN_FOR_SCENARIO);
    } // Y_UNIT_TEST(TestStandardPsn)
    Y_UNIT_TEST(TestScenePsn) {
        const TVector<ETestMode> modes = {
            ETestMode::ScenePsn_IntentDispatch,
            ETestMode::ScenePsn_IntentMain,
            ETestMode::ScenePsn_IntentRender
        };
        for (const auto it : modes) {
            TTestScenario testScenario(it);
            InitializeLocalScenario(testScenario);

            TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
            testEnv.AddSemanticFrameSlot(FRAME1_INTENT_NAME, "slot1", "value1");
            testEnv.AddSemanticFrameSlot(FRAME1_INTENT_NAME, "slot2", "value2");
            testEnv.AddSemanticFrameSlot(FRAME2_CUSTOM_NAME, "cslot1", "cvalue1");
            testEnv.AddSemanticFrameSlot(FRAME2_CUSTOM_NAME, "cslot2", "cvalue2");

            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

            // Validate that PSN is default scenario name
            UNIT_ASSERT(testEnv.ResponseBody.GetAnalyticsInfo().GetActions().size() == 0);
            UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetProductScenarioName(), PSN_FOR_SCENE);
            UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetIntent(), FRAME2_CUSTOM_NAME);

            if (it == ETestMode::ScenePsn_IntentRender) {
                UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetSemanticFrame().GetName(), FRAME1_INTENT_NAME);
            } else {
                UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetSemanticFrame().GetName(), FRAME2_CUSTOM_NAME);
            }
        }
    } // Y_UNIT_TEST(TestScenePsn)
    Y_UNIT_TEST(TestCustomPsn) {
        const TVector<ETestMode> modes = {
            ETestMode::CustomPsnDispatch,
            ETestMode::CustomPsnMain,
            ETestMode::CustomPsnRender
        };
        for (const auto it : modes) {
            TTestScenario testScenario(it);
            InitializeLocalScenario(testScenario);

            TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

            // Validate that PSN is default scenario name
            UNIT_ASSERT(testEnv.ResponseBody.GetAnalyticsInfo().GetActions().size() == 0);
            UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetProductScenarioName(), PSN_FOR_CUSTOM);
            UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetIntent(), FRAME1_INTENT_NAME);
        }
    } // Y_UNIT_TEST(TestScenePsn)
    Y_UNIT_TEST(TestActions) {
        TTestScenario testScenario(ETestMode::Actions);
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        // Validate final actions
        UNIT_ASSERT(testEnv.ResponseBody.GetAnalyticsInfo().GetActions().size() == 1);
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetActions()[0].GetId(), "Action");
        UNIT_ASSERT_STRINGS_EQUAL(testEnv.ResponseBody.GetAnalyticsInfo().GetActions()[0].GetName(), "ActionName");
    } // Y_UNIT_TEST(TestActions)

} // Y_UNIT_TEST_SUITE(HollywoodFrameworkSetupSource)

} // namespace
