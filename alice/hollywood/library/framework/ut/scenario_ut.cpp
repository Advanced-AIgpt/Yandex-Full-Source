#include "framework.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/proto/default_render.pb.h>
#include <alice/hollywood/library/framework/ut/proto/protos_ut.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw {

namespace {

constexpr TStringBuf SCENE_NAME1 = "scene1";
constexpr TStringBuf SCENE_NAME2 = "scene2";
constexpr TStringBuf SCENE_NAME3 = "scene3";
constexpr TStringBuf IRRELEVANT_TEXT = "Irrelevant answer";

TRetResponse RenderIrrelevant(const TProtoUtRenderer2&, TRender& render) {
    TTestRender::SetTextAnswer(render, TString{IRRELEVANT_TEXT});
    return TReturnValueSuccess();
}

} // anonimous namespace

//
// Complex scenario example
//
class TComplexScenarioScene1: public TScene<TProtoUtScene1> {
public:
    explicit TComplexScenarioScene1(const TScenario* owner)
        : TScene(owner, SCENE_NAME1)
    {
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest& request, TStorage&, const TSource&) const override;

    TRetSetup ContinueSetup(const TProtoUtScene1&, const TContinueRequest&, const TStorage&) const override;
    TRetContinue Continue(const TProtoUtScene1&, const TContinueRequest&, TStorage&, const TSource&) const override;

    TRetSetup ApplySetup(const TProtoUtScene1&, const TApplyRequest&, const TStorage&) const override;
    TRetContinue Apply(const TProtoUtScene1&, const TApplyRequest&, TStorage&, const TSource&) const override;
};

class TComplexScenarioScene2: public TScene<TProtoUtScene2> {
public:
    explicit TComplexScenarioScene2(const TScenario* owner)
        : TScene(owner, SCENE_NAME2)
    {
        RegisterRenderer(&TComplexScenarioScene2::Render3);
        RegisterRenderer(&TComplexScenarioScene2::Render4);
    }
    TRetMain Main(const TProtoUtScene2&, const TRunRequest&, TStorage&, const TSource&) const override;

    TRetSetup CommitSetup(const TProtoUtScene2&, const TCommitRequest&, const TStorage&) const override;
    TRetCommit Commit(const TProtoUtScene2&, const TCommitRequest&, TStorage&, const TSource&) const override;

    TRetResponse Render3(const TProtoUtRenderer3& renderArgs, TRender& render) const;
    static TRetResponse Render4(const TProtoUtRenderer3& renderArgs, TRender& render);
};

// Note Scene3 uses the same protobuf as Scene1
class TComplexScenarioScene3: public TScene<TProtoUtScene1> {
public:
    explicit TComplexScenarioScene3(const TScenario* owner)
        : TScene(owner, SCENE_NAME3)
    {
    }
    TRetMain Main(const TProtoUtScene1&, const TRunRequest&, TStorage&, const TSource&) const override;
    TRetResponse Render5(const TProtoUtRenderer3& renderArgs, TRender& render) const;
};


class TComplexScenario: public TScenario {
public:
    struct TOptions {
        enum class StageMode {
            Absent,
            MainOnly,
            MainAndSetup
        };
        int NodeCount;
        StageMode ContinueStage;
        StageMode CommitStage;
        StageMode ApplyStage;
        bool DebugLocalGraph;
    };
    TComplexScenario(const TOptions& options)
    : TScenario("complex_scenario") {
        if (options.DebugLocalGraph) {
            EnableDebugGraph();
        }
        // Generic scenario functions
        Register(&TComplexScenario::Dispatch);
        if (options.NodeCount == 3) {
            // Note DispatchSetup is incompatible with 1/2 node graphs
            // Trying a register DispatchSetup function with 1/2 node graphs will cause an exception
            Register(&TComplexScenario::DispatchSetup);
        }
        RegisterScene<TComplexScenarioScene1>([this, options]() {
            RegisterSceneFn(&TComplexScenarioScene1::Main);
            if (options.ContinueStage == TOptions::StageMode::MainOnly) {
                RegisterSceneFn(&TComplexScenarioScene1::Continue);
            }
            if (options.ContinueStage == TOptions::StageMode::MainAndSetup) {
                RegisterSceneFn(&TComplexScenarioScene1::ContinueSetup);
                RegisterSceneFn(&TComplexScenarioScene1::Continue);
            }
            if (options.ApplyStage == TOptions::StageMode::MainOnly) {
                RegisterSceneFn(&TComplexScenarioScene1::Apply);
            }
            if (options.ApplyStage == TOptions::StageMode::MainAndSetup) {
                RegisterSceneFn(&TComplexScenarioScene1::ApplySetup);
                RegisterSceneFn(&TComplexScenarioScene1::Apply);
            }
        });
        RegisterScene<TComplexScenarioScene2>([this, options]() {
            RegisterSceneFn(&TComplexScenarioScene2::Main);
            if (options.CommitStage == TOptions::StageMode::MainOnly) {
                RegisterSceneFn(&TComplexScenarioScene2::Commit);
            }
            if (options.CommitStage == TOptions::StageMode::MainAndSetup) {
                RegisterSceneFn(&TComplexScenarioScene2::CommitSetup);
                RegisterSceneFn(&TComplexScenarioScene2::Commit);
            }
        });
        RegisterScene<TComplexScenarioScene3>([this]() {
            RegisterSceneFn(&TComplexScenarioScene3::Main);
        });

        RegisterRenderer(&TComplexScenario::Render1);
        RegisterRenderer(&TComplexScenario::Render2);
        RegisterRenderer(&RenderIrrelevant);
        switch (options.NodeCount) {
            case 1:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                ScenarioResponse());
                break;
            case 2:
                SetApphostGraph(ScenarioRequest() >>
                                NodeRun() >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            case 3:
                SetApphostGraph(ScenarioRequest() >>
                                NodePreselect() >>
                                NodeRun() >>
                                NodeMain() >>
                                ScenarioResponse());
                break;
            default:
                Y_ENSURE(false, "Undefined node count");
        }
        if (options.ContinueStage != TOptions::StageMode::Absent) {
            SetApphostGraph(ScenarioContinue() >>
                            NodeContinueSetup() >>
                            NodeContinue() >>
                            ScenarioResponse());
        }
        if (options.ApplyStage != TOptions::StageMode::Absent) {
            SetApphostGraph(ScenarioApply() >>
                            NodeApplySetup() >>
                            NodeApply() >>
                            ScenarioResponse());
        }
        if (options.CommitStage != TOptions::StageMode::Absent) {
            SetApphostGraph(ScenarioCommit() >>
                            NodeCommitSetup() >>
                            NodeCommit() >>
                            ScenarioResponse());
        }
    }
    TRetSetup DispatchSetup(const TRunRequest& request, const TStorage&) const {
        if (request.System().RequestId() == "DispatchSetup->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received DispatchSetup->Error";
            return err;
        }
        return TSetup(request);
    }
    TRetScene Dispatch(const TRunRequest& request, const TStorage&, const TSource&) const {
        UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().ScenarioName, "complex_scenario");
        UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().NodeName, "run");
        UNIT_ASSERT_EQUAL(request.GetApphostInfo().NodeType, ENodeType::Run);

        if (request.System().RequestId() == "Dispatch->Irrelevant") {
            return TReturnValueRenderIrrelevant(&RenderIrrelevant, {});
        }
        if (request.System().RequestId() == "DispatchSetup->Dispatch->Irrelevant") {
            return TReturnValueRenderIrrelevant(&RenderIrrelevant, {});
        }
        if (request.System().RequestId() == "Dispatch->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->Error";
            return err;
        }
        if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene1::Run")) {
            return TReturnValueScene<TComplexScenarioScene1>(TProtoUtScene1{});
        }
        if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene3::Run")) {
            return TReturnValueScene<TComplexScenarioScene3>(TProtoUtScene1{});
        }
        if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::RunSetup")) {
            return TReturnValueScene<TComplexScenarioScene2>(TProtoUtScene2{});
        }
        if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::Run")) {
            return TReturnValueScene<TComplexScenarioScene2>(TProtoUtScene2{});
        }
        HW_ERROR("Impossible: '" << request.System().RequestId() << '\'');
    }

    static TRetResponse Render1(const TProtoUtRenderer1&, TRender& render) {
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->Render1->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->Render1->Error";
            return err;
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok") {
            TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok");
            return TReturnValueSuccess();
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error";
            return err;
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error";
            return err;
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok") {
            TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok");
            return TReturnValueSuccess();
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok") {
            TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok");
            return TReturnValueSuccess();
        }
        HW_ERROR("Impossible: " << render.GetRequest().System().RequestId());
    }
    TRetResponse Render1Alt(const TProtoUtRenderer1&, TRender&) const {
        return TReturnValueSuccess();
    }
    TRetResponse Render2(const TProtoUtRenderer2&, TRender& render) const {
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::RunSetup->Run->Render2 Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->TComplexScenarioScene2::RunSetup->Run->Render2 Error";
            return err;
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene3::Run->Render2->Error") {
            TError err(TError::EErrorDefinition::Timeout);
            err.Details() << "Received Dispatch->TComplexScenarioScene3::Run->Render2->Error";
            return err;
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::RunSetup->Run->Render2Ok") {
            TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene2::RunSetup->Run->Render2Ok");
            return TReturnValueSuccess();
        }
        if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene3::Run->Render2->Render Ok") {
            TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene3::Run->Render2->Render Ok");
            return TReturnValueSuccess();
        }
        HW_ERROR("Impossible: " << render.GetRequest().System().RequestId());
    }
};

// Scene1: without setup but with 'Continue' handler
TRetMain TComplexScenarioScene1::Main(const TProtoUtScene1&, const TRunRequest& request, TStorage&, const TSource&) const {
    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->Error";
        return err;
    }
    TRunFeatures features;
    features.IgnoresExpectedRequest(true);

    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue")) {
        TProtoUtContinue1 continueArgs;
        continueArgs.SetValue(1234);
        return TReturnValueContinue(continueArgs, std::move(features));
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply")) {
        TProtoUtContinue1 continueArgs;
        continueArgs.SetValue(4567);
        return TReturnValueApply(continueArgs, std::move(features));
    }
    return TReturnValueRender(&TComplexScenario::Render1, TProtoUtRenderer1{}, std::move(features));
}

TRetSetup TComplexScenarioScene1::ContinueSetup(const TProtoUtScene1&, const TContinueRequest& request, const TStorage&) const {
    TProtoUtContinue1 continueArgs;
    UNIT_ASSERT(request.GetArguments(continueArgs));
    UNIT_ASSERT_EQUAL(continueArgs.GetValue(), 1234);

    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().ScenarioName, "complex_scenario");
    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().NodeName, "continue_setup");
    UNIT_ASSERT_EQUAL(request.GetApphostInfo().NodeType, ENodeType::Continue);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ContinueSetup->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ContinueSetup->Error";
        return err;
    }
    return TSetup(request);
}
TRetContinue TComplexScenarioScene1::Continue(const TProtoUtScene1&, const TContinueRequest& request, TStorage&, const TSource&) const {
    TProtoUtContinue1 continueArgs;
    UNIT_ASSERT(request.GetArguments(continueArgs));
    UNIT_ASSERT_EQUAL(continueArgs.GetValue(), 1234);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error";
        return err;
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render")) {
        return TReturnValueRender(&TComplexScenario::Render1, TProtoUtRenderer1{});
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}

TRetSetup TComplexScenarioScene1::ApplySetup(const TProtoUtScene1&, const TApplyRequest& request, const TStorage&) const {
    TProtoUtContinue1 continueArgs;
    UNIT_ASSERT(request.GetArguments(continueArgs));
    UNIT_ASSERT_EQUAL(continueArgs.GetValue(), 4567);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ApplySetup->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ApplySetup->Error";
        return err;
    }
    return TSetup(request);
}
TRetContinue TComplexScenarioScene1::Apply(const TProtoUtScene1&, const TApplyRequest& request, TStorage&, const TSource&) const {
    TProtoUtContinue1 continueArgs;
    UNIT_ASSERT(request.GetArguments(continueArgs));
    UNIT_ASSERT_EQUAL(continueArgs.GetValue(), 4567);

    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().ScenarioName, "complex_scenario");
    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().NodeName, "apply");
    UNIT_ASSERT_EQUAL(request.GetApphostInfo().NodeType, ENodeType::Apply);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error";
        return err;
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render")) {
        return TReturnValueRender(&TComplexScenario::Render1, TProtoUtRenderer1{});
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}

TRetMain TComplexScenarioScene2::Main(const TProtoUtScene2&, const TRunRequest& request, TStorage&, const TSource&) const {
    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::Run->Error";
        return err;
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::Run->Render3")) {
        TProtoUtRenderer3 renderArgs;
        renderArgs.SetValue(521);
        auto rc =  TReturnValueRender(&TComplexScenarioScene2::Render3, renderArgs);
        return rc;
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit")) {
        TProtoUtCommit1 commitArgs;
        commitArgs.SetValue(7890);

        TProtoUtRenderer3 renderArgs;
        renderArgs.SetValue(1234);
        return TReturnValueCommit(&TComplexScenarioScene2::Render3, renderArgs, commitArgs);
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}
TRetSetup TComplexScenarioScene2::CommitSetup(const TProtoUtScene2&, const TCommitRequest& request, const TStorage&) const {
    TProtoUtCommit1 commitArgs;
    UNIT_ASSERT(request.GetArguments(commitArgs));
    UNIT_ASSERT_EQUAL(commitArgs.GetValue(), 7890);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::CommitSetup->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::CommitSetup->Error";
        return err;
    }
    if (request.System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit")) {
        return TSetup(request);
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}
TRetCommit TComplexScenarioScene2::Commit(const TProtoUtScene2&, const TCommitRequest& request, TStorage&, const TSource&) const {
    TProtoUtCommit1 commitArgs;
    UNIT_ASSERT(request.GetArguments(commitArgs));
    UNIT_ASSERT_EQUAL(commitArgs.GetValue(), 7890);

    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().ScenarioName, "complex_scenario");
    UNIT_ASSERT_STRINGS_EQUAL(request.GetApphostInfo().NodeName, "commit");
    UNIT_ASSERT_EQUAL(request.GetApphostInfo().NodeType, ENodeType::Commit);

    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Error";
        return err;
    }
    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error";
        return err;
    }
    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Ok") {
        return TReturnValueSuccess();
    }
    HW_ERROR("Impossible: " << request.System().RequestId());
}

TRetResponse TComplexScenarioScene2::Render3(const TProtoUtRenderer3& renderArgs, TRender& render) const {
    if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::RunSetup->Run->Render3 Error") {
        UNIT_ASSERT_EQUAL(renderArgs.GetValue(), 521);
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::RunSetup->Run->Render3 Error";
        return err;
    }
    if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::RunSetup->Run->Render3 Ok") {
        TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene2::RunSetup->Run->Render3 Ok");
        return TReturnValueSuccess();
    }
    if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->Render3->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene2::Run->Render3->Error";
        return err;
    }
    if (render.GetRequest().System().RequestId() == "Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok") {
        TTestRender::SetTextAnswer(render, "Received Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok");
        return TReturnValueSuccess();
    }
    if (render.GetRequest().System().RequestId().StartsWith("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit")) {
        TString textresponse = TString::Join("Received ", render.GetRequest().System().RequestId());
        TTestRender::SetTextAnswer(render, textresponse);
        return TReturnValueSuccess();
    }
    HW_ERROR("Impossible: " << render.GetRequest().System().RequestId());
}

TRetResponse TComplexScenarioScene2::Render4(const TProtoUtRenderer3&, TRender& render) {
    HW_ERROR("Impossible: " << render.GetRequest().System().RequestId());
}

TRetMain TComplexScenarioScene3::Main(const TProtoUtScene1&, const TRunRequest& request, TStorage&, const TSource&) const {
    if (request.System().RequestId() == "Dispatch->TComplexScenarioScene3::Run->Error") {
        TError err(TError::EErrorDefinition::Timeout);
        err.Details() << "Received Dispatch->TComplexScenarioScene3::Run->Error";
        return err;
    }
    return TReturnValueRender(&TComplexScenario::Render2, TProtoUtRenderer2{});
}

Y_UNIT_TEST_SUITE(ComplexScenarioTest) {

    Y_UNIT_TEST(ComplexScenario1Node) {
        TComplexScenario::TOptions options = {/* NodeCount */1,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID Dispatch->Irrelevant
            TTestEnvironment testResult(testEnv);
            testEnv.SetRequestId("Dispatch->Irrelevant");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            UNIT_ASSERT_STRINGS_EQUAL(testResult.GetText(), IRRELEVANT_TEXT);
        }
        {
            // Request ID Dispatch->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Render1->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Render1->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Render1->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Render3->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Render3->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->Render3->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok"));
        }
    }
    Y_UNIT_TEST(ComplexScenario2Node) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID Dispatch->Irrelevant
            TTestEnvironment testResult(testEnv);
            testEnv.SetRequestId("Dispatch->Irrelevant");
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT_STRINGS_EQUAL(testResult.GetText(), IRRELEVANT_TEXT);
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testEnv >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Render1->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Render1->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Render1->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->Render1->Render Ok"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Render3->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Render3->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->Render3->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene2::Run->Render3->Render Ok"));
        }
    }

    Y_UNIT_TEST(ComplexScenario3Node) {
        TComplexScenario::TOptions options = {/* NodeCount */3,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */  false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID DispatchSetup->Error
            TTestEnvironment testResult(testEnv);
            testEnv.SetRequestId("DispatchSetup->Error");
            testEnv.DisableErrorReporting();
            UNIT_ASSERT(!(testEnv >> TTestApphost("preselect") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received DispatchSetup->Error"));
        }
        {
            // Request ID DispatchSetup->Dispatch->Irrelevant
            TTestEnvironment testResult(testEnv);
            testEnv.SetRequestId("DispatchSetup->Dispatch->Irrelevant");
            UNIT_ASSERT(testEnv >> TTestApphost("preselect") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            UNIT_ASSERT_STRINGS_EQUAL(testResult.GetText(), IRRELEVANT_TEXT);
        }
        // All other cases already handled in ComplexScenario2Node
    }

    Y_UNIT_TEST(ComplexScenarioContinue1) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::MainOnly,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID DispatchSetup->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok"));
        }
    }

    Y_UNIT_TEST(ComplexScenarioContinue2) {
        TComplexScenario::TOptions options = {/* NodeCount */2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::MainAndSetup,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID DispatchSetup->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ContinueSetup->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ContinueSetup->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("continue_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ContinueSetup->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("continue") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue_setup") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("continue") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Continue->Render->Ok"));
        }
    }

    Y_UNIT_TEST(ComplexScenarioCommit1) {
        TComplexScenario::TOptions options = {/* NodeCount */2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::MainOnly,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID DispatchSetup->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("commit_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("commit") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("commit_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("commit") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Error
            TProtoUtCommit1 commitArgs;
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.UnpackCcaArguments(commitArgs));
            UNIT_ASSERT_EQUAL(commitArgs.GetValue(), 7890);
            UNIT_ASSERT(testResult >> TTestApphost("commit_setup") >> testResult);
            UNIT_ASSERT(testResult.UnpackCcaArguments(commitArgs));
            UNIT_ASSERT_EQUAL(commitArgs.GetValue(), 7890);
            UNIT_ASSERT(!(testResult >> TTestApphost("commit") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error"));
            UNIT_ASSERT(testResult >> TTestApphost("commit_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("commit") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene2::Run->TComplexScenarioScene2::Commit->Render->Ok"));
            UNIT_ASSERT(testResult >> TTestApphost("commit_setup") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("commit") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
        }
    }

    Y_UNIT_TEST(ComplexScenarioCommit2) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::MainAndSetup,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");
    }

    Y_UNIT_TEST(ComplexScenarioApply1) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::MainOnly,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");
        {
            // Request ID DispatchSetup->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok"));
        }
    }

    Y_UNIT_TEST(ComplexScenarioApply2) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::MainAndSetup,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID DispatchSetup->Error
            testEnv.SetRequestId("Dispatch->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(!(testResult >> TTestApphost("run") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            UNIT_ASSERT(testResult.IsIrrelevant());
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->Error"));
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ApplySetup->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ApplySetup->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("apply_setup") >> testResult));
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::ApplySetup->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("apply") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply_setup") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("apply") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene1::Run->TComplexScenarioScene1::Apply->Render->Ok"));
        }
    }

    Y_UNIT_TEST(ComplexScenarioSameProtos) {
        TComplexScenario::TOptions options = {/* NodeCount */ 2,
            /* ContinueStage   */ TComplexScenario::TOptions::StageMode::Absent,
            /* CommitStage     */ TComplexScenario::TOptions::StageMode::Absent,
            /* ApplyStage      */ TComplexScenario::TOptions::StageMode::Absent,
            /* DebugLocalGraph */ false};
        TComplexScenario complexScenario(options);
        InitializeLocalScenario(complexScenario);

        TTestEnvironment testEnv("complex_scenario", "ru-ru");
        testEnv.SetDeviceId("TESTDEVICEID");

        {
            // Request ID Dispatch->TComplexScenarioScene3::Run->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene3::Run->Error");
            testEnv.DisableErrorReporting();
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene3::Run->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene3::Run->Render1->Error
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene3::Run->Render2->Error");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(!(testResult >> TTestApphost("main") >> testResult));
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() != TError::EErrorDefinition::Success);
            TString errorMsg = testResult.GetErrorInfo().GetDetails();
            UNIT_ASSERT(errorMsg.Contains("Received Dispatch->TComplexScenarioScene3::Run->Render2->Error"));
            UNIT_ASSERT(testResult.IsIrrelevant());
        }
        {
            // Request ID Dispatch->TComplexScenarioScene3::Run->Render1->Render Ok
            testEnv.SetRequestId("Dispatch->TComplexScenarioScene3::Run->Render2->Render Ok");
            TTestEnvironment testResult(testEnv);
            UNIT_ASSERT(testEnv >> TTestApphost("run") >> testResult);
            UNIT_ASSERT(testResult >> TTestApphost("main") >> testResult);
            UNIT_ASSERT(testResult.GetErrorInfo().GetErrorCode() == TError::EErrorDefinition::Success);
            UNIT_ASSERT(!testResult.IsIrrelevant());
            UNIT_ASSERT(testResult.ContainsText("Received Dispatch->TComplexScenarioScene3::Run->Render2->Render Ok"));
        }
    }
}

} // namespace NAlice::NHollywoodFw
