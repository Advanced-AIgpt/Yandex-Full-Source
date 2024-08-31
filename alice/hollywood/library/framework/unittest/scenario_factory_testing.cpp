//
// HOLLYWOOD FRAMEWORK
// Internal class : scenario factory
//

#include "test_globalcontext.h"

#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>
#include <alice/hollywood/library/framework/core/scene_graph.h>
#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/fwd.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw {

namespace {

struct TExternalRunRequest {
    TExternalRunRequest(const TTestEnvironment& testData)
        : Logger(TRTLogger::StderrLogger())
        , NodeInfo({testData.GetScenarioName(), "", NJson::TJsonValue::UNDEFINED, ENodeType::Run})
        , GlobalCtx(testData)
        , RunRequest(testData.RequestMeta,
            testData.RunRequest,
            Ctx,
            nullptr,
            GlobalCtx,
            Logger,
            NodeInfo,
            /*Nlg*/ Nothing())
    {
    }
    TRTLogger& Logger;
    NAppHost::NService::TTestContext Ctx;
    TRequest::TApphostNodeInfo NodeInfo;
    NPrivate::TTestGlobalContext GlobalCtx;
    TRunRequest RunRequest;
};

} // anonimous namespace

void NPrivate::TScenarioFactory::EnsureScenarioInitialization(const TString& scenarioName) {
    Y_ENSURE(!IsHollywoodApp_, "This function can be used in test app only!");
    auto it = AllScenarios_.find(scenarioName);
    if (it != AllScenarios_.end() && it->second->InitializationStageFlag_) {
        it->second->FinishInitialization(nullptr, "");
    }
}

/*
    Initialize scenario declared as a local variable
    This functon is needed if you declare your scenario handle in the unit test function
    instead of HW_REGISTER() macro.
    If you test you scenario directly, you don't need to use this call.
*/
void InitializeLocalScenario(TScenario& localScenarioVar) {
    NPrivate::TScenarioFactory::Instance().EnsureScenarioInitialization(localScenarioVar.GetName());
}

namespace NPrivate {

const TRunRequest& TScenarioFactory::CreateRunRequest(const TTestEnvironment& testData) {
    static std::unique_ptr<TExternalRunRequest> TestRunRequest;
    TestRunRequest.reset(new TExternalRunRequest(testData));
    return TestRunRequest->RunRequest;
}

/*
    Perform a test call for scenario
    This function can be used in unit tests only!
*/
void TScenarioFactory::DispatchScenarioHandleUt(const TTestEnvironment& testData,
                                                TNodeCaller& caller,
                                                NHollywood::IGlobalContext& globalContext) {
    Y_ENSURE(!IsHollywoodApp_, "This function can be used in test app only!");
    Y_ENSURE(caller.GetApphostNode() != nullptr, "Aphost node is not defined");
    CallLocalGraph(caller, testData.RequestMeta.GetRandomSeed(), globalContext.Sensors());
    Y_ENSURE(!caller.IsSwitchToOldFlow(), "Switching to old flow is not supported in unit tests");
}

} // namespace NPrivate

} // namespace NAlice::NHollywoodFw
