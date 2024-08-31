#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

constexpr TStringBuf SCENE_NAME1 = "scene1";

class TSEScenarioScene1: public TScene<TProtoUtScene1> {
public:
    explicit TSEScenarioScene1(const TScenario* owner)
        : TScene(owner, SCENE_NAME1)
    {
        RegisterRenderer(&TSEScenarioScene1::Render);
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest& request, TStorage&, const TSource&) const override;
    TRetResponse Render(const TProtoUtRenderer1&, TRender& render) const;
};

class TSEScenario: public TScenario {
public:
    TSEScenario()
    : TScenario("se_scenario") {
        Register(&TSEScenario::Dispatch);
        RegisterScene<TSEScenarioScene1>([this]() {
            RegisterSceneFn(&TSEScenarioScene1::Main);
        });
    }
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        // Does nothing switch directly to test scene
        return TReturnValueScene<TSEScenarioScene1>(TProtoUtScene1{});
    }
};

// Scene1: without setup but with 'Continue' handler
TRetMain TSEScenarioScene1::Main(const TProtoUtScene1& args, const TRunRequest& request, TStorage& storage, const TSource& source) const {
    Y_UNUSED(args);
    Y_UNUSED(request);
    Y_UNUSED(storage);
    Y_UNUSED(source);
    return TReturnValueRender(&TSEScenarioScene1::Render, {});
}

TRetResponse TSEScenarioScene1::Render(const TProtoUtRenderer1& renderArgs, TRender& render) const {
    Y_UNUSED(renderArgs);
    NScenarios::TStackEngine se;
    *se.AddActions()->MutableNewSession() = NScenarios::TStackEngineAction::TNewSession{};
    render.SetStackEngine(std::move(se));
    return TReturnValueSuccess();
}

} // anonimous namespace

Y_UNIT_TEST_SUITE(SeScenarioTest) {

    Y_UNIT_TEST(SeTest) {
        TSEScenario seScenario;
        InitializeLocalScenario(seScenario);

        TTestEnvironment testEnv("se_scenario", "ru-ru");
        TTestEnvironment testResult(testEnv);
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
        UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
        // Validate that the final response has StackEngine parameters
        const auto& se = testResult.ResponseBody.GetStackEngine();
        UNIT_ASSERT(se.GetActions().size() == 1);
        UNIT_ASSERT(se.GetActions()[0].HasNewSession());
    } // Y_UNIT_TEST(SeTest)
} // Y_UNIT_TEST_SUITE(SeScenarioTest)

} // namespace NAlice::NHollywoodFw
