#include "speechkit_utils.h"

#include <alice/library/client/client_info.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/experiments/utils.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/megamind/library/response/utils.h>

#include <util/datetime/base.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

#include <type_traits>

namespace NAlice::NWonderlogs {

namespace {

template <typename T>
typename std::enable_if<std::is_base_of_v<google::protobuf::Message, T>, NJson::TJsonValue>::type
CopyField(const T& t) {
    return JsonFromProto(t);
}

template <typename T>
typename std::enable_if<!std::is_base_of_v<google::protobuf::Message, T>, NJson::TJsonValue>::type
CopyField(const T& t) {
    return t;
}

} // namespace

NJson::TJsonValue TVinsLikeRequest::DumpJson() const {
    NJson::TJsonValue vinsLikeRequest;
    vinsLikeRequest["dialog_id"] = NJson::JSON_NULL;
    if (DialogId) {
        vinsLikeRequest["dialog_id"] = *DialogId;
    }
    vinsLikeRequest["request_id"] = RequestId;
    vinsLikeRequest["prev_req_id"] = PrevReqId;
    vinsLikeRequest["sequence_number"] = SequenceNumber;

    vinsLikeRequest["uuid"] = Uuid;
    vinsLikeRequest["device_id"] = DeviceId;
    vinsLikeRequest["lang"] = Lang;
    vinsLikeRequest["client_time"] = ClientTime;

    if (CallbackArgs) {
        vinsLikeRequest["callback_args"] = JsonFromProto(*CallbackArgs);
    } else {
        vinsLikeRequest["callback_args"].SetType(NJson::EJsonValueType::JSON_MAP);
    }
    vinsLikeRequest["callback_name"].SetType(NJson::EJsonValueType::JSON_NULL);

    NJson::TJsonValue eventJson;
    eventJson["payload"].SetType(NJson::EJsonValueType::JSON_NULL);

    switch (eventType) {
        case text_input: {
            [[fallthrough]];
        }
        case suggested_input: {
            eventJson["input_source"] = Utterance.InputSource;
            eventJson["end_of_utterance"] = *Utterance.EndOfUtterance;
            eventJson["text"] = *Utterance.Text;
            break;
        }
        case voice_input: {
            eventJson["input_source"] = Utterance.InputSource;
            eventJson["end_of_utterance"] = NJson::JSON_NULL;
            if (Utterance.EndOfUtterance) {
                eventJson["end_of_utterance"] = *Utterance.EndOfUtterance;
            }
            eventJson["hypothesis_number"] = NJson::JSON_NULL;
            if (Utterance.HypothesisNumber) {
                eventJson["hypothesis_number"] = *Utterance.HypothesisNumber;
            }
            if (Utterance.Text) {
                eventJson["text"] = *Utterance.Text;
            }
            break;
        }
        case server_action: {
            vinsLikeRequest["callback_name"] = *CallbackName;
            eventJson.SetType(NJson::JSON_NULL);
            break;
        }
        case image_input: {
            eventJson["input_source"] = Utterance.InputSource;
            eventJson["payload"]["data"] = JsonFromProto(Utterance.Payload.Data);
            eventJson["text"].SetType(NJson::JSON_NULL);
            break;
        }
        case music_input: {
            eventJson["input_source"] = Utterance.InputSource;
            eventJson["text"].SetType(NJson::JSON_NULL);

            NJson::TJsonValue& payloadJson = eventJson["payload"];
            payloadJson["data"] = JsonFromProto(Utterance.Payload.Data);
            payloadJson["result"] = Utterance.Payload.Result;
            payloadJson["error_text"] = Utterance.Payload.ErrorText;
            break;
        }
    }

    vinsLikeRequest["utterance"] = eventJson;

    if (Location) {
        vinsLikeRequest["location"] = JsonFromProto(*Location);
    }

    if (ResetSession) {
        vinsLikeRequest["reset_session"] = *ResetSession;
    }

    if (VoiceSession) {
        vinsLikeRequest["voice_session"] = *VoiceSession;
    }

    if (LaasRegion) {
        vinsLikeRequest["laas_region"] = JsonFromProto(*LaasRegion);
    }

    if (Experiments) {
        NMegamind::TExpFlagsToJsonVisitor{vinsLikeRequest["experiments"]}.Visit(*Experiments);
    }

    NJson::TJsonValue& testIds = vinsLikeRequest["test_ids"].SetType(NJson::EJsonValueType::JSON_ARRAY);
    for (const auto id : TestIDs) {
        testIds.AppendValue(id);
    }

    NJson::TJsonValue& appInfoJson = vinsLikeRequest["app_info"];
    appInfoJson["app_id"] = AppInfo.GetAppId();
    appInfoJson["app_version"] = AppInfo.GetAppVersion();
    appInfoJson["platform"] = AppInfo.GetPlatform();
    appInfoJson["os_version"] = AppInfo.GetOsVersion();
    appInfoJson["device_manufacturer"] = AppInfo.GetDeviceManufacturer();
    appInfoJson["device_model"] = AppInfo.GetDeviceModel();
    appInfoJson["device_revision"] = AppInfo.GetDeviceRevision();

    vinsLikeRequest["device_state"] = JsonFromProto(DeviceState);

    vinsLikeRequest["srcrwr"].SetType(NJson::EJsonValueType::JSON_MAP);

    vinsLikeRequest["additional_options"] = JsonFromProto(AdditionalOptions);

    if (EnvironmentState) {
        for (const auto& device : *EnvironmentState) {
            vinsLikeRequest["enviroment_state"].AppendValue(JsonFromProto(device));
        }
    }

    return vinsLikeRequest;
};

TVinsLikeRequest::TVinsLikeRequest(const TSpeechKitRequestProto& skRequest) {
    const auto& event = skRequest.GetRequest().GetEvent();
    const auto& appProto = skRequest.GetApplication();
    const auto& headerProto = skRequest.GetHeader();
    const auto& requestProto = skRequest.GetRequest();

    if (headerProto.HasDialogId()) {
        DialogId = headerProto.GetDialogId();
    }
    RequestId = headerProto.GetRequestId();
    PrevReqId = headerProto.GetPrevReqId();
    SequenceNumber = headerProto.GetSequenceNumber();
    Uuid = SplitID(appProto.GetUuid());
    DeviceId = SplitID(appProto.GetDeviceId());
    Lang = appProto.GetLang();
    ClientTime = ToString(appProto.GetEpoch());

    if (event.HasPayload()) {
        CallbackArgs = event.GetPayload();
    }

    eventType = event.GetType();
    TVinsLikeRequest::TEvent eventStruct;

    switch (eventType) {
        case text_input: {
            eventStruct.InputSource = "text";
            eventStruct.EndOfUtterance = true;
            eventStruct.Text = event.GetText();
            break;
        }
        case suggested_input: {
            eventStruct.InputSource = "suggested";
            eventStruct.EndOfUtterance = true;
            eventStruct.Text = event.GetText();
            break;
        }
        case voice_input: {
            eventStruct.InputSource = "voice";
            if (event.HasEndOfUtterance()) {
                eventStruct.EndOfUtterance = event.GetEndOfUtterance();
            }
            if (event.HasHypothesisNumber()) {
                eventStruct.HypothesisNumber = event.GetHypothesisNumber();
            }
            if (event.AsrResultSize()) {
                const auto& bestResult = event.GetAsrResult(0);
                if (bestResult.WordsSize()) {
                    const auto& bestWords = bestResult.GetWords();
                    TStringBuilder utterance;
                    for (const auto& elem : bestWords) {
                        if (!utterance.Empty()) {
                            utterance << ' ';
                        }
                        utterance << elem.GetValue();
                    }
                    eventStruct.Text = utterance;
                } else {
                    eventStruct.Text = bestResult.GetNormalized();
                }
            }
            break;
        }
        case server_action: {
            CallbackName = event.GetName();
            break;
        }
        case image_input: {
            eventStruct.InputSource = "image";
            TVinsLikeRequest::TEvent::TPayload payload;
            payload.Data = event.GetPayload();
            eventStruct.Payload = payload;
            break;
        }
        case music_input: {
            eventStruct.InputSource = "music";

            const auto& musicResult = event.GetMusicResult();
            TVinsLikeRequest::TEvent::TPayload payload;
            payload.Data = MessageToStruct(musicResult.GetData());
            payload.Result = musicResult.GetResult();
            payload.ErrorText = musicResult.GetErrorText();
            eventStruct.Payload = payload;
            break;
        }
    }

    Utterance = eventStruct;
    if (requestProto.HasLocation()) {
        Location = requestProto.GetLocation();
    }
    if (requestProto.HasResetSession()) {
        ResetSession = requestProto.GetResetSession();
    }
    if (requestProto.HasVoiceSession()) {
        VoiceSession = requestProto.GetVoiceSession();
    }
    if (requestProto.HasLaasRegion()) {
        LaasRegion = requestProto.GetLaasRegion();
    }
    if (requestProto.HasExperiments()) {
        Experiments = requestProto.GetExperiments();
    }

    for (const auto& id : requestProto.GetTestIDs()) {
        TestIDs.push_back(id);
    }

    AppInfo = appProto;
    DeviceState = skRequest.GetRequest().GetDeviceState();
    AdditionalOptions = skRequest.GetRequest().GetAdditionalOptions();

    const auto& envDevices = requestProto.GetEnvironmentState().GetDevices();
    TVector<TEnvironmentDeviceInfo> devices(Reserve(envDevices.size()));
    for (const auto& device : envDevices) {
        devices.push_back(device);
    }

    if (!devices.empty()) {
        EnvironmentState = std::move(devices);
    }

    ServerTimeMs = skRequest.GetRequest().GetAdditionalOptions().GetServerTimeMs();
}

TVinsLikeResponse::TVinsLikeResponse(const TSpeechKitResponseProto& skResponse)
    : TVinsLikeResponse([](const TSpeechKitResponseProto& skResponse) {
        return NAlice::SpeechKitResponseToJson(skResponse);
    }(skResponse)) {
}

TVinsLikeResponse::TVinsLikeResponse(const NJson::TJsonValue& skResponse) {
    if (!skResponse["response"].IsDefined() || skResponse["response"] == "...") {
        Empty = true;
        return;
    }

    if (!skResponse["response"].Has("force_voice_answer")) {
        ForceVoiceAnswer = false;
    }

    ShouldListen = skResponse["voice_response"]["should_listen"].GetBoolean();

    if (const auto& voiceText = skResponse["voice_response"]["output_speech"]["text"]; voiceText.IsDefined()) {
        VoiceText = voiceText.GetString();
    }

    if (skResponse["response"].Has("special_buttons")) {
        SpecialButtons = skResponse["response"]["special_buttons"];
    }

    if (const auto& items = skResponse["response"]["suggest"]["items"]; items.IsDefined()) {
        Suggests = items;
    }
    if (skResponse["response"].Has("templates")) {
        Templates = skResponse["response"]["templates"];
    }
    if (skResponse["response"].Has("quality_storage")) {
        QualityStorage = skResponse["response"]["quality_storage"];
    }
    if (skResponse["response"].Has("experiments")) {
        Experiments = skResponse["response"]["experiments"];
    }
    if (skResponse["response"].Has("directives_execution_policy")) {
        DirectivesExecutionPolicy = skResponse["response"]["directives_execution_policy"].GetString();
    }
    if (skResponse["response"].Has("cards")) {
        Cards = skResponse["response"]["cards"];
    }
    if (skResponse["response"].Has("directives")) {
        Directives = skResponse["response"]["directives"];
    }
    if (skResponse.Has("contains_sensitive_data")) {
        ContainsSensitiveData = skResponse["contains_sensitive_data"].GetBoolean();
    }
    if (skResponse["header"].Has("response_id")) {
        ResponseId = skResponse["header"]["response_id"].GetString();
    }
}

NJson::TJsonValue TVinsLikeResponse::DumpJson() const {
    if (Empty) {
        return NJson::JSON_NULL;
    }
    NJson::TJsonValue response;
    response["force_voice_answer"] = ForceVoiceAnswer;
    response["should_listen"] = ShouldListen;
    response["voice_text"] = NJson::JSON_NULL;
    if (VoiceText) {
        response["voice_text"] = *VoiceText;
    }
    response["special_buttons"] = SpecialButtons;
    response["suggests"] = Suggests;
    if (Templates) {
        response["templates"] = *Templates;
    }
    if (QualityStorage) {
        response["quality_storage"] = *QualityStorage;
    }
    if (Experiments) {
        response["experiments"] = *Experiments;
    }
    if (DirectivesExecutionPolicy) {
        response["directives_execution_policy"] = *DirectivesExecutionPolicy;
    }
    if (Cards) {
        response["cards"] = *Cards;
    }
    if (Directives) {
        response["directives"] = *Directives;
    }

    // TODO delete
    if (auto* metas = response.GetValueByPath("meta"); metas && metas->IsArray()) {
        for (auto& meta : metas->GetArraySafe()) {
            if (auto* form = meta.GetValueByPath("form")) {
                (*form)["slots"].SetType(NJson::JSON_ARRAY);
                for (auto& slot : (*form)["slots"].GetArraySafe()) {
                    slot = NJson::ReadJsonFastTree(slot.GetString());
                }
            }
        }
    }
    // TODO delete
    if (auto* directives = response.GetValueByPath("directives"); directives && directives->IsArray()) {
        for (auto& directive : directives->GetArraySafe()) {
            if (auto& payload = directive["payload"]; !payload.IsDefined()) {
                payload.SetType(NJson::JSON_NULL);
            }
        }
    }

    return response;
}

TVinsLikeLog::TVinsLikeLog(const TSpeechKitRequestProto& skRequest, const TSpeechKitResponseProto& skResponse)
    : SpeechkitRequest(skRequest)
    , SpeechkitResponse(skResponse) {
}

NJson::TJsonValue TVinsLikeLog::DumpJson() const {
    NJson::TJsonValue vinsLikeLog;

    const TVinsLikeRequest vinsLikeRequest(SpeechkitRequest);
    const TVinsLikeResponse vinsLikeResponse(SpeechkitResponse);

    vinsLikeLog["app_id"] = "pa";
    vinsLikeLog["client_time"] = FromString<ui64>(vinsLikeRequest.ClientTime);
    vinsLikeLog["client_tz"] = vinsLikeRequest.AppInfo.GetTimezone();
    vinsLikeLog["response_id"] = vinsLikeResponse.ResponseId;

    if (vinsLikeResponse.ContainsSensitiveData && *vinsLikeResponse.ContainsSensitiveData) {
        vinsLikeLog["contains_sensitive_data"] = true;
    }
    vinsLikeLog["response"] = vinsLikeResponse.DumpJson();

    vinsLikeLog["analytics_info"] = JsonFromProto(SpeechkitResponse.GetMegamindAnalyticsInfo());

    vinsLikeLog["provider"] = "megamind";

    vinsLikeLog["type"] = "UTTERANCE";
    if (vinsLikeRequest.CallbackName) {
        vinsLikeLog["type"] = "CALLBACK";
    }

    vinsLikeLog["server_time"] = TInstant::MilliSeconds(vinsLikeRequest.ServerTimeMs).Seconds();
    vinsLikeLog["server_time_ms"] = vinsLikeRequest.ServerTimeMs;

    {
        const auto vinsLikeRequestJson = vinsLikeRequest.DumpJson();
        vinsLikeLog["request"] = vinsLikeRequestJson;
        vinsLikeLog["utterance_source"] = vinsLikeRequestJson["utterance"]["input_source"];
        vinsLikeLog["utterance_text"] = vinsLikeRequestJson["utterance"]["text"];
    }

    vinsLikeLog["uuid"] = vinsLikeRequest.Uuid;

    vinsLikeLog["callback_name"] = NJson::EJsonValueType::JSON_NULL;
    if (vinsLikeRequest.CallbackName) {
        vinsLikeLog["callback_name"] = *vinsLikeRequest.CallbackName;
    }

    vinsLikeLog["callback_args"] = NJson::EJsonValueType::JSON_MAP;
    if (vinsLikeRequest.CallbackArgs) {
        vinsLikeLog["callback_args"] = JsonFromProto(*vinsLikeRequest.CallbackArgs);
    }

    vinsLikeLog["location_lat"] = NJson::EJsonValueType::JSON_NULL;
    vinsLikeLog["location_lon"] = NJson::EJsonValueType::JSON_NULL;

    if (vinsLikeRequest.Location) {
        vinsLikeLog["location_lat"] = vinsLikeRequest.Location->GetLat();
        vinsLikeLog["location_lon"] = vinsLikeRequest.Location->GetLon();
    }

    vinsLikeLog["lang"] = vinsLikeRequest.Lang;
    if (vinsLikeRequest.Experiments) {
        NMegamind::TExpFlagsToJsonVisitor{vinsLikeLog["experiments"]}.Visit(*vinsLikeRequest.Experiments);
    }
    vinsLikeLog["device_id"] = vinsLikeRequest.DeviceId;

    if (vinsLikeLog["experiments"].Has(NExperiments::DUMP_SESSIONS_TO_LOGS)) {
        vinsLikeLog["session"] = SpeechkitRequest.HasSession() ? SpeechkitRequest.GetSession() : NJson::TJsonValue{};
    }

    vinsLikeLog["biometry_scoring"] = JsonFromProto(SpeechkitRequest.GetRequest().GetEvent().GetBiometryScoring());
    vinsLikeLog["biometry_classification"] =
        JsonFromProto(SpeechkitRequest.GetRequest().GetEvent().GetBiometryClassification());
    return vinsLikeLog;
}

} // namespace NAlice::NWonderlogs
