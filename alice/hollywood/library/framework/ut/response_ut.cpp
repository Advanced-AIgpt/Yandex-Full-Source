#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>
#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>

#include <alice/megamind/protos/scenarios/frame.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

class TTestScene: public TScene<TProtoUtScene1> {
public:
    TTestScene(const TScenario* owner)
        : TScene(owner, "test_scene")
    {
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override{
        HW_ERROR("Impossible");
    }
};

class TTestScenario: public TScenario {
public:
    TTestScenario()
        : TScenario("my_test_scenario")
    {
        Register(&TTestScenario::Dispatch);
        RegisterScene<TTestScene>([this]() {
            RegisterSceneFn(&TTestScene::Main);
        });
        RegisterRenderer(&TTestScenario::Render1);
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
    }
private:
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        return TReturnValueRenderIrrelevant(&TTestScenario::Render1, {});
    }
    static TRetResponse Render1(const TProtoUtRenderer1&, TRender& render) {
        render.Directives().AddStartBluetoothDirective();
        render.Directives().AddStopBluetoothDirective();

        NAlice::NScenarios::TTtsPlayPlaceholderDirective directive;
        directive.SetName("tts_play_placeholder_directive");
        render.Directives().AddTtsPlayPlaceholderDirective(std::move(directive));
        return TReturnValueSuccess();
    }
};

class TEllipsisScenario: public TScenario {
public:
    TEllipsisScenario()
        : TScenario("my_test_scenario")
    {
        Register(&TEllipsisScenario::Dispatch);
        RegisterScene<TTestScene>([this]() {
            RegisterSceneFn(&TTestScene::Main);
        });
        RegisterRenderer(&TEllipsisScenario::Render);
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
    }
private:
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        return TReturnValueRenderIrrelevant(&TEllipsisScenario::Render, {});
    }
    static TRetResponse Render(const TProtoUtRenderer1&, TRender& render) {
        TTypedSemanticFrame tsf;
        tsf.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("shla sasha to shosse i sosalla sushku");

        render.AddEllipsisFrame("super.duper", std::move(tsf));
        render.AddEllipsisFrame("super.puper", std::move(tsf), "my_action_name");
        render.AddEllipsisFrame("duper.super");
        render.AddEllipsisFrame("pupper.super", "my_action_duper_super");
        return TReturnValueSuccess();
    }
};

} // anonimous namespace

Y_UNIT_TEST_SUITE(ResponseTests) {

    Y_UNIT_TEST(DirectivesTest) {
        TTestScenario testScenario;
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);

        const auto& directives = testEnv.ResponseBody.GetLayout().GetDirectives();
        UNIT_ASSERT(directives.size() == 3);
        UNIT_ASSERT(directives[0].HasStartBluetoothDirective());
        UNIT_ASSERT(directives[1].HasStopBluetoothDirective());
        UNIT_ASSERT(directives[2].HasTtsPlayPlaceholderDirective());

        UNIT_ASSERT_STRINGS_EQUAL(directives[0].GetStartBluetoothDirective().GetName(), "start_bluetooth_directive");
        UNIT_ASSERT_STRINGS_EQUAL(directives[1].GetStopBluetoothDirective().GetName(), "stop_bluetooth_directive");
        UNIT_ASSERT_STRINGS_EQUAL(directives[2].GetTtsPlayPlaceholderDirective().GetName(), "tts_play_placeholder_directive");
    }

    Y_UNIT_TEST(AddEllipsisFrameTsfWithoutName) {
        TEllipsisScenario testScenario;
        InitializeLocalScenario(testScenario);

        TTestEnvironment testEnv(testScenario.GetName(), "ru-ru");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testEnv);
        const auto& fa = testEnv.ResponseBody.GetFrameActions();
        UNIT_ASSERT_VALUES_EQUAL(fa.size(), 4);

        const auto item1 = fa.find("ellipsis_0");
        UNIT_ASSERT(item1 != fa.cend());
        UNIT_ASSERT_VALUES_EQUAL(item1->second.GetNluHint().GetFrameName(), "super.duper");
        UNIT_ASSERT_VALUES_EQUAL(item1->second.GetParsedUtterance().GetTypedSemanticFrame().GetSearchSemanticFrame().GetQuery().GetStringValue(), "shla sasha to shosse i sosalla sushku");

        const auto item2 = fa.find("ellipsis_2");
        UNIT_ASSERT(item2 != fa.cend());
        UNIT_ASSERT_VALUES_EQUAL(item2->second.GetNluHint().GetFrameName(), "duper.super");

        const auto item3 = fa.find("my_action_name");
        UNIT_ASSERT(item3 != fa.cend());
        UNIT_ASSERT_VALUES_EQUAL(item3->second.GetNluHint().GetFrameName(), "super.puper");

        const auto item4 = fa.find("my_action_duper_super");
        UNIT_ASSERT(item4 != fa.cend());
        UNIT_ASSERT_VALUES_EQUAL(item4->second.GetNluHint().GetFrameName(), "pupper.super");
    }
}

} // namespace NAlice::NHollywoodFw
