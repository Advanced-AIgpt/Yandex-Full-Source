#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/protos/api/renderer/api.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

constexpr TStringBuf SCENE_NAME1 = "scene1";

enum class ConstructorMode {
    OneNode,
    TwoNodeWoRender,
    TwoNodeRender,
    ThreeNodeRender
};

/*
    Convert text DevRender request to DivCard

*/
void ConvertDivRender2Card(TTestEnvironment& env) {
    UNIT_ASSERT_EQUAL(env.DivRenderData.size(), 1);

    for (const auto& it : env.DivRenderData) {
        NAlice::NRenderer::TRenderResponse resp;
        resp.SetCardName(it->GetCardId());
        env.DivRenderResponse.push_back(std::make_shared<NRenderer::TRenderResponse>(std::move(resp)));
    }
}

//
// DivRender scenario example
//
class TDivRenderScenarioScene1: public TScene<TProtoUtScene1> {
public:
    explicit TDivRenderScenarioScene1(const TScenario* owner)
        : TScene(owner, SCENE_NAME1)
    {
        RegisterRenderer(&TDivRenderScenarioScene1::Render);
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest& request, TStorage&, const TSource&) const override;
    TRetResponse Render(const TProtoUtRenderer1&, TRender& render) const;

    TRetContinue Continue(const TProtoUtScene1&, const TContinueRequest&, TStorage&, const TSource&) const override;
    TRetContinue Apply(const TProtoUtScene1&, const TApplyRequest&, TStorage&, const TSource&) const override;
    TRetCommit Commit(const TProtoUtScene1&, const TCommitRequest&, TStorage&, const TSource&) const override;
};

class TDivRenderScenario: public TScenario {
public:
    TDivRenderScenario(ConstructorMode mode)
    : TScenario("divrender_scenario") {
        // Generic scenario functions
        Register(&TDivRenderScenario::Dispatch);
        RegisterScene<TDivRenderScenarioScene1>([this]() {
            RegisterSceneFn(&TDivRenderScenarioScene1::Main);
            RegisterSceneFn(&TDivRenderScenarioScene1::Continue);
            RegisterSceneFn(&TDivRenderScenarioScene1::Apply);
            RegisterSceneFn(&TDivRenderScenarioScene1::Commit);
        });

        switch (mode) {
            case ConstructorMode::OneNode:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioContinue() >>
                                NodeContinue() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioApply() >>
                                NodeApply() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioCommit() >>
                                NodeCommit() >>
                                ScenarioResponse());
                break;
            case ConstructorMode::TwoNodeWoRender:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeMain() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioContinue() >>
                                NodeContinueSetup() >>
                                NodeContinue() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioApply() >>
                                NodeApplySetup() >>
                                NodeApply() >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioCommit() >>
                                NodeCommitSetup() >>
                                NodeCommit() >>
                                ScenarioResponse());
                break;
            case ConstructorMode::TwoNodeRender:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun("run", "experiment1") >>
                                NodeRender("render") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeRender("render1") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioContinue() >>
                                NodeContinue() >>
                                NodeRender("render2") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioApply() >>
                                NodeApply() >>
                                NodeRender("render3") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioCommit() >>
                                NodeCommit() >>
                                ScenarioResponse());
                break;
            case ConstructorMode::ThreeNodeRender:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRender("render") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeMain() >>
                                NodeRender("render1") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioContinue() >>
                                NodeContinueSetup() >>
                                NodeContinue() >>
                                NodeRender("render2") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioApply() >>
                                NodeApplySetup() >>
                                NodeApply() >>
                                NodeRender("render3") >>
                                ScenarioResponse());
                SetApphostGraph(ScenarioCommit() >>
                                NodeCommit() >>
                                ScenarioResponse());
                break;
            default:
                Y_ENSURE(false, "Undefined node count");
        }
    }
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const {
        // Does nothing switch directly to test scene
        return TReturnValueScene<TDivRenderScenarioScene1>(TProtoUtScene1{});
    }
};

// Scene1: without setup but with 'Continue' handler
TRetMain TDivRenderScenarioScene1::Main(const TProtoUtScene1& args, const TRunRequest& request, TStorage&, const TSource&) const {
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId(request.System().RequestId());

    if (request.System().RequestId() == "SimpleRun") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {});
    }
    if (request.System().RequestId() == "DivRun") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {}).AddDivRender(std::move(renderData));
    }
    if (request.System().RequestId() == "SimpleRunWithContinue" || request.System().RequestId() == "DivRunWithContinue") {
        return TReturnValueContinue(args);
    }
    if (request.System().RequestId() == "SimpleRunWithApply" || request.System().RequestId() == "DivRunWithApply") {
        return TReturnValueApply(args);
    }
    if (request.System().RequestId() == "SimpleRunWithCommit") {
        return TReturnValueCommit(&TDivRenderScenarioScene1::Render, {}, args);
    }
    if (request.System().RequestId() == "DivRunWithCommit") {
        return TReturnValueCommit(&TDivRenderScenarioScene1::Render, {}, args).AddDivRender(std::move(renderData));
    }
    HW_ERROR("Impossible: '" << request.System().RequestId() << '\'');
}

TRetContinue TDivRenderScenarioScene1::Continue(const TProtoUtScene1&, const TContinueRequest& request, TStorage&, const TSource&) const {
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId(request.System().RequestId());
    if (request.System().RequestId() == "DivRunWithContinue") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {}).AddDivRender(std::move(renderData));
    }
    if (request.System().RequestId() == "SimpleRunWithContinue") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {});
    }
    HW_ERROR("Impossible: '" << request.System().RequestId() << '\'');
}

TRetContinue TDivRenderScenarioScene1::Apply(const TProtoUtScene1&, const TApplyRequest& request, TStorage&, const TSource&) const {
    NRenderer::TDivRenderData renderData;
    renderData.SetCardId(request.System().RequestId());

    if (request.System().RequestId() == "DivRunWithApply") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {}).AddDivRender(std::move(renderData));
    }
    if (request.System().RequestId() == "SimpleRunWithApply") {
        return TReturnValueRender(&TDivRenderScenarioScene1::Render, {});
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}
TRetCommit TDivRenderScenarioScene1::Commit(const TProtoUtScene1&, const TCommitRequest& request, TStorage&, const TSource&) const {
    if (request.System().RequestId() == "DivRunWithCommit") {
        return TReturnValueSuccess();
    }
    if (request.System().RequestId() == "SimpleRunWithCommit") {
        return TReturnValueSuccess();
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}

TRetResponse TDivRenderScenarioScene1::Render(const TProtoUtRenderer1& renderArgs, TRender& render) const {
    Y_UNUSED(renderArgs);
    if (render.GetRequest().System().RequestId().StartsWith("DivRun")) {
        const auto& div = render.GetDivRenderResponse();
        UNIT_ASSERT_EQUAL(div.size(), 1);
        UNIT_ASSERT_STRINGS_EQUAL(div[0]->GetCardName(), render.GetRequest().System().RequestId());
    }
    return TReturnValueSuccess();
}

/*
    Additional test function to handle both TwoNodeRender and ThreeNodeRender cases
    Called from Y_UNIT_TEST(DivRenderScenario2NodeRender) and Y_UNIT_TEST(DivRenderScenario3NodeRender)
*/
void CheckDivRender(ConstructorMode mode) {
    TDivRenderScenario complexScenario(mode);
    InitializeLocalScenario(complexScenario);

    TTestEnvironment testEnv("divrender_scenario", "ru-ru");
    TTestEnvironment testResult(testEnv);
    {
        testEnv.SetRequestId("DivRun");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
        if (mode == ConstructorMode::ThreeNodeRender) {
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
        }
        UNIT_ASSERT(!testResult.DivRenderData.empty());
        ConvertDivRender2Card(testResult);
        UNIT_ASSERT(testResult >> TTestApphost("render1") >> testResult);

    }
    {
        testEnv.SetRequestId("DivRunWithContinue");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
        if (mode == ConstructorMode::ThreeNodeRender) {
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("render") >> testResult);
        }
        UNIT_ASSERT(testResult >> TTestApphost("continue") >> testResult);
        UNIT_ASSERT(!testResult.DivRenderData.empty());
        ConvertDivRender2Card(testResult);
        UNIT_ASSERT(testResult >> TTestApphost("render2") >> testResult);
    }
    {
        testEnv.SetRequestId("DivRunWithApply");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
        if (mode == ConstructorMode::ThreeNodeRender) {
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("render") >> testResult);
        }
        UNIT_ASSERT(testResult >> TTestApphost("apply") >> testResult);
        UNIT_ASSERT(!testResult.DivRenderData.empty());
        ConvertDivRender2Card(testResult);
        UNIT_ASSERT(testResult >> TTestApphost("render") >> testResult);
    }
    {
        testEnv.SetRequestId("DivRunWithCommit");
        UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
        if (mode == ConstructorMode::ThreeNodeRender) {
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);

        }
        UNIT_ASSERT(!testResult.DivRenderData.empty());
        ConvertDivRender2Card(testResult);
        UNIT_ASSERT(testResult >> TTestApphost("render1") >> testResult);
        UNIT_ASSERT(testResult >> TTestApphost("commit") >> testResult);
    }
}


} // anonimous namespace

Y_UNIT_TEST_SUITE(DivRenderScenarioDivRenderTest) {

    Y_UNIT_TEST(DivRenderScenario1Node) {
        TDivRenderScenario complexScenario(ConstructorMode::OneNode);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("divrender_scenario", "ru-ru");
        TTestEnvironment testResult(testEnv);
        {
            testEnv.SetRequestId("SimpleRun");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithContinue");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithApply");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithCommit");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("commit") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
    }
    Y_UNIT_TEST(DivRenderScenario2Node) {
        TDivRenderScenario complexScenario(ConstructorMode::TwoNodeWoRender);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("divrender_scenario", "ru-ru");
        TTestEnvironment testResult(testEnv);
        {
            testEnv.SetRequestId("SimpleRun");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithContinue");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithApply");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
        {
            testEnv.SetRequestId("SimpleRunWithCommit");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("commit") >> testResult);
            UNIT_ASSERT(testResult.DivRenderData.empty());
        }
    }
    Y_UNIT_TEST(DivRenderScenario2NodeRender) {
        CheckDivRender(ConstructorMode::TwoNodeRender);
    }
    Y_UNIT_TEST(DivRenderScenario3NodeRender) {
        // CheckDivRender(ConstructorMode::ThreeNodeRender);
    }
}

} // namespace NAlice::NHollywoodFw
