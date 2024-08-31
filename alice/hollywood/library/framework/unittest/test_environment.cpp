#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/framework/core/node_caller.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>


#include <alice/library/json/json.h>
#include <alice/library/unittest/mock_sensors.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/json/json_reader.h>

namespace NAlice::NHollywoodFw {

namespace {

TSemanticFrame* FindOfCreateSf(NScenarios::TInput* mutableInput, TStringBuf frameName) {
    // Check and find this semantic frame in Input
    for (auto& it : *mutableInput->MutableSemanticFrames()) {
        if (it.GetName() == frameName) {
            return &it;
        }

    }
    TSemanticFrame* sf = mutableInput->AddSemanticFrames();
    sf->SetName(frameName.Data());
    return sf;
}

} // anonimous namespace

TTestEnvironment::TTestEnvironment(TStringBuf scenarioName, TStringBuf lang)
    : ScenarioName_(scenarioName)
{
    static bool firstCallFlag = true;
    if (firstCallFlag) {
        // Need to create and initialize all existing scenarios
        NPrivate::TScenarioFactory::Instance().CreateAllScenarios();
        firstCallFlag = false;
    }

    RequestMeta.SetLang(lang.Data());
    RequestMeta.SetUserLang(lang.Data());
}

TTestEnvironment::~TTestEnvironment() = default;

/*
    Prepare data for initial test call (TTestEnvironment >> TestFn() >> TTestEnvironment)
                                                        ^^^^
    Where TestFn is an any test class based on TTestBase
*/
const NPrivate::TTestBase& TTestEnvironment::operator >> (const NPrivate::TTestBase& rhs) const {
    rhs.TestEnvironment = this;
    return rhs;
}

/*
 * Helper functions to set initial values of TTestEnvironment
 */

/*
    Set device identifier for source request
    Helper function call instead of direct access to initial protobuf request
*/
bool TTestEnvironment::SetDeviceId(TStringBuf deviceId) {
    RunRequest.MutableBaseRequest()->MutableDeviceState()->SetDeviceId(deviceId.Data());
    return true;
}

/*
    Add semantic frame into the source request
    Helper function call instead of direct access to initial protobuf request
*/
bool TTestEnvironment::AddSemanticFrame(TStringBuf frameName, TStringBuf frameJson, TStringBuf utterance /*= TStringBuf("")*/) {
    NJson::TJsonValue jsonData;
    if (!NJson::ReadJsonFastTree(frameJson, &jsonData, /* throwOnError = */ false)) {
        return false;
    }

    TSemanticFrame* sf = FindOfCreateSf(RunRequest.MutableInput(), frameName);
    for (const auto& it : jsonData.GetArray()) {
        TSemanticFrame::TSlot slot;
        slot.SetName(it["name"].GetString());
        slot.SetType(it["type"].GetString());
        slot.SetValue(it["value"].GetString());
        sf->AddSlots()->CopyFrom(slot);
    }
    RunRequest.MutableInput()->MutableText()->SetUtterance(utterance.Data());
    return true;
}

void TTestEnvironment::AddSemanticFrameSlot(TStringBuf frameName, TStringBuf slotName, const TString& value) {
    TSemanticFrame* sf = FindOfCreateSf(RunRequest.MutableInput(), frameName);
    TSemanticFrame::TSlot slot;
    slot.SetName(slotName.Data());
    slot.SetType("string");
    slot.SetValue(value);
    sf->AddSlots()->CopyFrom(slot);
}

void TTestEnvironment::AddSemanticFrameSlot(TStringBuf frameName, TStringBuf slotName, const int value) {
    TSemanticFrame* sf = FindOfCreateSf(RunRequest.MutableInput(), frameName);
    TSemanticFrame::TSlot slot;
    slot.SetName(slotName.Data());
    slot.SetType("int");
    slot.SetValue(ToString(value));
    sf->AddSlots()->CopyFrom(slot);
}


/*
    Add experiment to sourcerequest
    Helper function call instead of direct access to initial protobuf request
*/
bool TTestEnvironment::AddExp(TStringBuf exp) {
    google::protobuf::Struct* experiments = RunRequest.MutableBaseRequest()->MutableExperiments();
    google::protobuf::Value val;
    val.set_null_value(google::protobuf::NullValue());
    (*experiments->mutable_fields())[exp.Data()] = val;
    return true;
}
bool TTestEnvironment::AddExp(TStringBuf exp, TStringBuf value) {
    google::protobuf::Struct* experiments = RunRequest.MutableBaseRequest()->MutableExperiments();
    google::protobuf::Value val;
    val.set_string_value(value.Data());
    (*experiments->mutable_fields())[exp.Data()] = val;
    return true;
}

/*
    Set request identifier for source request
    Helper function call instead of direct access to initial protobuf request
*/
bool TTestEnvironment::SetRequestId(TStringBuf requestId) {
    RequestMeta.SetRequestId(requestId.Data());
    return true;
}

void TTestEnvironment::AddPUID(const TString& puid) {
    auto* bb = (*RunRequest.MutableDataSources())[NAlice::EDataSourceType::BLACK_BOX].MutableUserInfo();
    bb->SetUid(puid);
}

void TTestEnvironment::AddAnswer(TStringBuf key, TStringBuf jsonString) {
    SourceAnswersJson[TString(key)] = JsonFromString(jsonString);
}

void TTestEnvironment::AddHttpAnswer(TStringBuf key, const NAppHostHttp::THttpResponse& resp) {
    google::protobuf::Message* msg = new NAppHostHttp::THttpResponse;
    msg->CopyFrom(resp);
    SourceAnswersProto[TString(key)].reset(msg);
}

/*
    Create a copy of TRunRequest
    This function can be used to support unit tests for old scenarios
*/
const TRunRequest& TTestEnvironment::CreateRunRequest() const {
    return NPrivate::TScenarioFactory::Instance().CreateRunRequest(*this);
}

const TString& TTestEnvironment::GetSelectedSceneName() const {
    if (ProtoHwScene_.HasSceneArgs()) {
        return ProtoHwScene_.GetSceneArgs().GetSceneName();
    }
    Y_ENSURE(false, "Scene arguments was not set, selected scene name is missing");
}

const TString& TTestEnvironment::GetSelectedIntent() const {
    if (ProtoHwScene_.HasSceneArgs()) {
        return ProtoHwScene_.GetSceneArgs().GetSemanticFrameName();
    }
    Y_ENSURE(false, "Scene arguments was not set, selected intent is missing");
}

const google::protobuf::Any& TTestEnvironment::GetSelectedSceneArguments() const {
    if (ProtoHwScene_.HasSceneArgs()) {
        return ProtoHwScene_.GetSceneArgs().GetArgs();
    }
    Y_ENSURE(false, "Scene arguments was not set, required data are missing");
}
TError TTestEnvironment::GetErrorInfo() const {
    return TError(ProtoHwScene_.GetError());
}

/*
 * Helper functions to check result data saved in TTestEnvironment
 */

/*
    Check that the result text contains this phrase
*/
bool TTestEnvironment::ContainsText(TStringBuf text) const {
    const auto cards = ResponseBody.GetLayout().GetCards();
    for (const auto& it : cards) {
        const TString str = it.GetText();
        if (str.Contains(text)) {
            return true;
        }
    }
    return false;
}

/*
    Get result text
*/
const TString TTestEnvironment::GetText() const {
    TString result;
    const auto cards = ResponseBody.GetLayout().GetCards();
    for (const auto& it : cards) {
        result += it.GetText();
    }
    return result;
}

/*
    Get result voice
*/
const TString TTestEnvironment::GetVoice() const {
    return ResponseBody.GetLayout().GetOutputSpeech();
}

/*
    Check that the result voice contains this phrase
*/
bool TTestEnvironment::ContainsVoice(TStringBuf voice) const {
    const TString str = ResponseBody.GetLayout().GetOutputSpeech();
    return str.Contains(voice);
}

/*
    Check is render result is irrelevant
*/
bool TTestEnvironment::IsIrrelevant() const {
    return RunResponseFeartures.GetIsIrrelevant();
}

const google::protobuf::Message* TTestEnvironment::FindSetupRequest(TStringBuf outgoingName) const {
    if (SetupRequests == nullptr) {
        return nullptr;
    }
    return SetupRequests->FindMessage(outgoingName);
}

} // namespace NAlice::NHollywoodFw
