#pragma once

//
// HOLLYWOOD FRAMEWORK TESTS
// Test Environment
//

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <memory>

namespace NAlice::NRenderer {

class TDivRenderData;
class TRenderResponse;

} // namespace NAlice::NRenderer

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
namespace NPrivate {

class TTestBase;

} // namespace NPrivate


//
// All test data to call scenario nodes, inspect results and pass intermediate results into the next nodes
//
class TTestEnvironment {
public:
    // Test environment ctor
    TTestEnvironment(TStringBuf scenarioName, TStringBuf lang);
    ~TTestEnvironment();
    const TString& GetScenarioName() const {
        return ScenarioName_;
    }
    // Getting TRunRequest for old scenarios (for easy migrations in unit tests)
    const TRunRequest& CreateRunRequest() const;

    // Prevent from dumping error message into log/Stderr
    void DisableErrorReporting() {
        ErrorReporting_ = false;
    }
    bool IsErrorReporting() const {
        return ErrorReporting_;
    }

    //
    // Operations for RunRequest / RequestMeta
    //
    bool SetDeviceId(TStringBuf deviceId);
    bool AddSemanticFrame(TStringBuf frameName, TStringBuf frameJson, TStringBuf utterance = TStringBuf(""));
    void AddSemanticFrameSlot(TStringBuf frameName, TStringBuf slotName, const TString& value);
    void AddSemanticFrameSlot(TStringBuf frameName, TStringBuf slotName, int value);
    // Add an experiment with non-defined value
    bool AddExp(TStringBuf exp);
    // Add an experiment with value
    bool AddExp(TStringBuf exp, TStringBuf value);
    // Operations for RequestMeta
    bool SetRequestId(TStringBuf requestId);
    // Operations for blackbox
    void AddPUID(const TString& puid);

    //
    // Mock fastdata support
    //
    void AttachFastdata(std::shared_ptr<IFastData> fastData) {
        FastData_.push_back(fastData);
    }
    inline const TVector<std::shared_ptr<IFastData>>& GetFastData() const {
        return FastData_;
    }

    //
    // Operations with scene arguments, render arguments and other output info
    //
    const TString& GetSelectedSceneName() const;
    const TString& GetSelectedIntent() const;
    const google::protobuf::Any& GetSelectedSceneArguments() const;
    TProtoHwScene& GetProtoHwScene() {
        return ProtoHwScene_;
    }
    TError GetErrorInfo() const;
    const TString GetText() const;
    const TString GetVoice() const;
    bool ContainsText(TStringBuf text) const;
    bool ContainsVoice(TStringBuf voice) const;
    bool IsIrrelevant() const;

    // Mock answers for TSource object
    void AddAnswer(TStringBuf key, TStringBuf jsonValue);
    void AddHttpAnswer(TStringBuf key, const NAppHostHttp::THttpResponse& resp);
    template <class TProto>
    void AddAnswer(TStringBuf key, const TProto& resp) {
        auto copy = std::make_shared<TProto>();
        copy->CopyFrom(resp);
        SourceAnswersProto[TString(key)] = copy;
    }

    // Helpers to test output results
    const google::protobuf::Message* FindSetupRequest(TStringBuf outgoingName) const;

    // Helper to test continue/commit/apply arguments
    template <class TArgs>
    bool UnpackCcaArguments(TArgs& proto) {
        return ContinueCommitApplyArgs.UnpackTo(&proto);
    }

    // Operations to call scenario flow functions
    const NPrivate::TTestBase& operator >> (const NPrivate::TTestBase& rhs) const;

public:
    // Generic input data, used to construct TRequest/TStorage and other framework classes
    NScenarios::TRequestMeta RequestMeta;
    NScenarios::TScenarioRunRequest RunRequest;
    google::protobuf::Any ContinueCommitApplyArgs;

    // TSetup/TSource data
    std::shared_ptr<TSetup> SetupRequests;
    TMap<TString, NJson::TJsonValue> SourceAnswersJson;
    TMap<TString, std::shared_ptr<google::protobuf::Message>> SourceAnswersProto;

    // Output results
    NScenarios::TScenarioResponseBody ResponseBody;
    TVector<std::shared_ptr<NRenderer::TDivRenderData>> DivRenderData;
    TVector<std::shared_ptr<NRenderer::TRenderResponse>> DivRenderResponse;
    // Additional data available in TScenarioRunResponse/TScenarioContinueResponse/...
    NScenarios::TScenarioRunResponse::TFeatures  RunResponseFeartures;
    NScenarios::TUserInfo  RunRequestUserInfo;
    // Error - see GetErrorInfo();
    // ApplyArguments, ContinueArguments - see ContinueCommitApplyArgs
    // CommitCandidate - see ContinueCommitApplyArgs + ResponseBody
private:
    TString ScenarioName_;
    TProtoHwScene ProtoHwScene_;
    bool ErrorReporting_ = true;
    TVector<std::shared_ptr<IFastData>> FastData_;
};

} // namespace NAlice::NHollywoodFw
