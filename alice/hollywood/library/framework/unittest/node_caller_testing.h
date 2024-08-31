//
// HOLLYWOOD FRAMEWORK
//
//

#pragma once

#include "test_environment.h"
#include "test_globalcontext.h"
#include <alice/hollywood/library/framework/core/node_caller.h>

#include <apphost/lib/service_testing/service_testing.h>

namespace NAlice::NHollywoodFw::NPrivate {

//
// Special TNodeCaller class for testing purposes
//
class TNodeCallerTesting: public TNodeCaller {
public:
    TNodeCallerTesting(const TScenario& sc, const TString& nodeName, TTestEnvironment& env);
    TNodeCallerTesting(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env);

    void Bypass() override;
    void Finalize() override;
    bool HasThisStage() const override;
    NAppHost::NService::TTestContext& GetCtx() {
        return TestCtx_;
    }
    TTestGlobalContext& GetGlobalCtx() {
        return GlobalCtx_;
    }
    NScenarios::TScenarioApplyRequest& ConvertRunRequest(const TTestEnvironment& env);
    const NScenarios::TScenarioBaseRequest& GetBaseRequestProto() const override {
        return TestEnv_.RunRequest.GetBaseRequest();
    }
    const NScenarios::TInput* GetInputProto() const override {
        return TestEnv_.RunRequest.HasInput() ? &TestEnv_.RunRequest.GetInput() : nullptr;
    }
    NScenarios::TScenarioResponseBody* GetBaseResponseProto() override {
        return &TestEnv_.ResponseBody;
    }

protected:
    TTestEnvironment& TestEnv_;
    EStageDeclaration StageToTest_;
    NAppHost::NService::TTestContext TestCtx_;
    TTestGlobalContext GlobalCtx_;

private:
    NScenarios::TScenarioApplyRequest FakeApplyRequest_;

private:
    void InitializeTestData();
    void SaveAllToTestEnv();
};

//
// Special TNodeCaller class for testing purposes
//
class TNodeCallerTestingRun: public TNodeCallerTesting {
public:
    TNodeCallerTestingRun(const TScenario& sc, const TString& nodeName, TTestEnvironment& env);
    TNodeCallerTestingRun(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env);

    ENodeType GetNodeType() const override {
        return ENodeType::Run;
    }
    const TRunRequest& GetRunRequest() const override {
        return RunRequest_;
    }
    const TRequest& GetBaseRequest() const override {
        return RunRequest_;
    }

private:
    TRunRequest RunRequest_;
};

class TNodeCallerTestingContinue: public TNodeCallerTesting {
public:
    TNodeCallerTestingContinue(const TScenario& sc, const TString& nodeName, TTestEnvironment& env);
    TNodeCallerTestingContinue(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env);

    ENodeType GetNodeType() const override {
        return ENodeType::Continue;
    }
    const TContinueRequest& GetContinueRequest() const override {
        return ContinueRequest_;
    }
    const TRequest& GetBaseRequest() const override {
        return ContinueRequest_;
    }
private:
    TContinueRequest ContinueRequest_;
};

class TNodeCallerTestingApply: public TNodeCallerTesting {
public:
    TNodeCallerTestingApply(const TScenario& sc, const TString& nodeName, TTestEnvironment& env);
    TNodeCallerTestingApply(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env);

    ENodeType GetNodeType() const override {
        return ENodeType::Apply;
    }
    const TApplyRequest& GetApplyRequest() const override {
        return ApplyRequest_;
    }
    const TRequest& GetBaseRequest() const override {
        return ApplyRequest_;
    }

private:
    TApplyRequest ApplyRequest_;
};

class TNodeCallerTestingCommit: public TNodeCallerTesting {
public:
    TNodeCallerTestingCommit(const TScenario& sc, const TString& nodeName, TTestEnvironment& env);
    TNodeCallerTestingCommit(const TScenario& sc, EStageDeclaration stage, TTestEnvironment& env);

    ENodeType GetNodeType() const override {
        return ENodeType::Commit;
    }
    const TCommitRequest& GetCommitRequest() const override {
        return CommitRequest_;
    }
    const TRequest& GetBaseRequest() const override {
        return CommitRequest_;
    }

private:
    TCommitRequest CommitRequest_;
};

} // namespace NAlice::NHollywoodFw::NPrivate
