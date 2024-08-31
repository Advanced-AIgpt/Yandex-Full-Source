#include "dialogs.h"

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/request_stat.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <alice/library/client/client_info.h>
#include <alice/library/json/json.h>
#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/interface/client_method_options.h>

#include <util/charset/utf8.h>
#include <util/datetime/base.h>
#include <util/string/builder.h>

namespace {

const ui32 MAX_UTTERANCE_SIZE = 2000;

} // namespace

namespace NAlice::NWonderlogs {

TDialogsBuilder::TDialogsBuilder(const TMaybe<TEnvironment>& productionEnvironment)
    : ProductionEnvironment_(productionEnvironment) {
}

TDialog::TDialog(const TWonderlog& wonderlog)
    : Request(wonderlog.GetSpeechkitRequest())
    , Response(wonderlog.GetSpeechkitResponse()) {
    Uuid = "uu/" + NormalizeUuid(wonderlog.GetUuid());
    ServerTime = TInstant::MilliSeconds(wonderlog.GetServerTimeMs()).Seconds();
    ServerTimeMs = wonderlog.GetServerTimeMs();
    SequenceNumber = wonderlog.GetSpeechkitRequest().GetHeader().GetSequenceNumber();
    MegamindAnalyticsInfo = wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo();
    if (wonderlog.GetSpeechkitRequest().GetRequest().GetEvent().HasBiometryScoring()) {
        BiometryScoring = wonderlog.GetSpeechkitRequest().GetRequest().GetEvent().GetBiometryScoring();
    }
    if (wonderlog.GetSpeechkitRequest().GetRequest().GetEvent().HasBiometryClassification()) {
        BiometryClassification = wonderlog.GetSpeechkitRequest().GetRequest().GetEvent().GetBiometryClassification();
    }

    CallbackName = Request.CallbackName;
    CallbackArgs = Request.CallbackArgs;
    if (TryFromString(wonderlog.GetSpeechkitRequest().GetApplication().GetEpoch(), ClientTime)) {
        // TODO(ran1s) do something
    }
    ClientTz = wonderlog.GetSpeechkitRequest().GetApplication().GetTimezone();
    ContainsSensitiveData = wonderlog.GetSpeechkitResponse().GetContainsSensitiveData();
    DeviceId = SplitID(wonderlog.GetSpeechkitRequest().GetApplication().GetDeviceId());
    DeviceRevision = wonderlog.GetSpeechkitRequest().GetApplication().GetDeviceRevision();
    if (wonderlog.GetSpeechkitRequest().GetHeader().HasDialogId()) {
        DialogId = wonderlog.GetSpeechkitRequest().GetHeader().GetDialogId();
    }
    DoNotUseUserLogs = wonderlog.GetPrivacy().GetDoNotUseUserLogs();
    Experiments = wonderlog.GetSpeechkitRequest().GetRequest().GetExperiments();

    if (!wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo().HasWinnerScenario() &&
        !wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetWinnerScenario().GetName().empty()) {
        FormName = wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetWinnerScenario().GetName();
    } else if (!wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetAnalyticsInfo().empty() &&
               !wonderlog.GetSpeechkitResponse()
                    .GetMegamindAnalyticsInfo()
                    .GetAnalyticsInfo()
                    .begin()
                    ->first.empty()) {
        FormName = wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo().GetAnalyticsInfo().begin()->first;
    } else if (Request.CallbackName) {
        FormName = "";
    }
    if (FormName) {
        Form = NJson::EJsonValueType::JSON_MAP;
        Form["form"] = *FormName;
        Form["slots"] = NJson::EJsonValueType::JSON_ARRAY;
    }
    Lang = wonderlog.GetSpeechkitRequest().GetApplication().GetLang();
    if (wonderlog.GetSpeechkitRequest().GetRequest().HasLocation()) {
        LocationLat = wonderlog.GetSpeechkitRequest().GetRequest().GetLocation().GetLat();
        LocationLon = wonderlog.GetSpeechkitRequest().GetRequest().GetLocation().GetLon();
    }
    MessageId = wonderlog.GetMessageId();
    if (wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().HasPuid()) {
        Puid = wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetPuid();
    }
    RequestId = wonderlog.GetMegamindRequestId();
    if (wonderlog.HasRequestStat()) {
        RequestStat = NJson::EJsonValueType::JSON_MAP;
        RequestStat["timestamps"] = NJson::EJsonValueType::JSON_MAP;
        auto& timestampsJson = RequestStat["timestamps"];
        const auto& timestampsProto = wonderlog.GetRequestStat().GetTimestamps();
        if (timestampsProto.HasOnSoundPlayerEndTime()) {
            timestampsJson["onSoundPlayerEndTime"] = timestampsProto.GetOnSoundPlayerEndTime();
        }
        if (timestampsProto.HasOnVinsResponseTime()) {
            timestampsJson["onVinsResponseTime"] = timestampsProto.GetOnVinsResponseTime();
        }
        if (timestampsProto.HasOnInterruptionPhraseSpottedTime()) {
            timestampsJson["onInterruptionPhraseSpottedTime"] = timestampsProto.GetOnInterruptionPhraseSpottedTime();
        }
        if (timestampsProto.HasOnFirstSynthesisChunkTime()) {
            timestampsJson["onFirstSynthesisChunkTime"] = timestampsProto.GetOnFirstSynthesisChunkTime();
        }
        if (timestampsProto.HasOnRecognitionBeginTime()) {
            timestampsJson["onRecognitionBeginTime"] = timestampsProto.GetOnRecognitionBeginTime();
        }
        if (timestampsProto.HasOnRecognitionEndTime()) {
            timestampsJson["onRecognitionEndTime"] = timestampsProto.GetOnRecognitionEndTime();
        }
        if (timestampsProto.HasRequestDurationTime()) {
            timestampsJson["requestDurationTime"] = timestampsProto.GetRequestDurationTime();
        }
        if (timestampsProto.HasSpotterConfirmationTime()) {
            timestampsJson["spotterConfirmationTime"] = timestampsProto.GetSpotterConfirmationTime();
        }
        if (timestampsProto.HasOnPhraseSpottedTime()) {
            timestampsJson["onPhraseSpottedTime"] = timestampsProto.GetOnPhraseSpottedTime();
        }
    }
    ResponseId = wonderlog.GetSpeechkitResponse().GetHeader().GetResponseId();
    SessionId = wonderlog.GetSpeechkitRequest().GetHeader().GetSessionId();
    Type = "UTTERANCE";
    if (CallbackName) {
        Type = "CALLBACK";
    }
    {
        const auto vinsLikeRequestJson = Request.DumpJson();
        if (vinsLikeRequestJson["utterance"].Has("input_source") &&
            vinsLikeRequestJson["utterance"]["input_source"].IsString()) {
            UtteranceSource = vinsLikeRequestJson["utterance"]["input_source"].GetString();
        }
        if (vinsLikeRequestJson["utterance"].Has("text") && vinsLikeRequestJson["utterance"]["text"].IsString()) {
            UtteranceText = vinsLikeRequestJson["utterance"]["text"].GetString();
        }
    }
    TrashOrEmptyRequest = wonderlog.GetAsr().GetTrashOrEmpty();
    if (wonderlog.GetSpeechkitRequest().HasEnrollmentHeaders()) {
        EnrollmentHeaders = wonderlog.GetSpeechkitRequest().GetEnrollmentHeaders();
    }
    if (wonderlog.GetSpeechkitRequest().HasGuestUserData()) {
        GuestData = wonderlog.GetSpeechkitRequest().GetGuestUserData();
    }
}

NYT::TNode TDialog::DumpNode() const {
    NYT::TNode dialog;
    // TODO(make GetValueByPath)
    // https://st.yandex-team.ru/MEGAMIND-1682
    dialog["request"] = NYT::NodeFromJsonString(ToString(Request.DumpJson()));
    dialog["response"] = NYT::ENodeType::NT_MAP;
    {
        const auto response = Response.DumpJson();
        if (response.GetType() == NJson::JSON_MAP) {
            // https://st.yandex-team.ru/MEGAMIND-1682
            dialog["response"] = NYT::NodeFromJsonString(ToString(response));
        }
    }
    dialog["uuid"] = Uuid;
    if (LocationLat) {
        dialog["location_lat"] = *LocationLat;
    }
    if (LocationLon) {
        dialog["location_lon"] = *LocationLon;
    }
    dialog["dialog_id"] = NYT::TNode::CreateEntity();
    if (DialogId) {
        dialog["dialog_id"] = *DialogId;
    }
    dialog["utterance_text"] = NYT::TNode::CreateEntity();
    if (UtteranceText) {
        dialog["utterance_text"] = SubstrUTF8(*UtteranceText, 0, MAX_UTTERANCE_SIZE);
    }

    dialog["server_time"] = ServerTime;
    dialog["server_time_ms"] = ServerTimeMs;

    dialog["sequence_number"] = SequenceNumber;
    dialog["message_id"] = MessageId;
    dialog["lang"] = Lang;
    dialog["device_id"] = NYT::TNode::CreateEntity();
    if (DeviceId) {
        dialog["device_id"] = *DeviceId;
    }
    dialog["device_revision"] = NYT::TNode::CreateEntity();
    if (DeviceRevision) {
        dialog["device_revision"] = *DeviceRevision;
    }
    dialog["request_stat"] = NYT::NodeFromJsonString(ToString(RequestStat));
    dialog["type"] = Type;
    dialog["utterance_source"] = NYT::TNode::CreateEntity();
    if (UtteranceSource) {
        dialog["utterance_source"] = *UtteranceSource;
    }
    dialog["callback_name"] = NYT::TNode::CreateEntity();
    if (CallbackName) {
        dialog["callback_name"] = *CallbackName;
    }
    dialog["callback_args"] = NYT::TNode::CreateMap();
    if (CallbackArgs) {
        dialog["callback_args"] = NYT::NodeFromJsonString(JsonStringFromProto(*CallbackArgs));
    }
    dialog["contains_sensitive_data"] = ContainsSensitiveData;
    dialog["analytics_info"] = NYT::NodeFromJsonString(JsonStringFromProto(MegamindAnalyticsInfo));
    {
        NJson::TJsonValue experimentsJson;
        NMegamind::TExpFlagsToJsonVisitor{experimentsJson}.Visit(Experiments);
        // https://st.yandex-team.ru/MEGAMIND-1682
        dialog["experiments"] = NYT::NodeFromJsonString(ToString(experimentsJson));
    }
    dialog["puid"] = NYT::TNode::CreateEntity();
    if (Puid) {
        dialog["puid"] = *Puid;
    }
    dialog["request_id"] = RequestId;
    dialog["client_tz"] = ClientTz;
    dialog["client_time"] = ClientTime;
    dialog["response_id"] = ResponseId;

    dialog["session_id"] = NYT::TNode::CreateEntity();
    if (SessionId) {
        dialog["session_id"] = *SessionId;
    }
    dialog["do_not_use_user_logs"] = DoNotUseUserLogs;
    dialog["form_name"] = NYT::TNode::CreateEntity();
    if (FormName) {
        dialog["form_name"] = *FormName;
    }
    dialog["form"] = NYT::NodeFromJsonString(ToString(Form));
    dialog["biometry_classification"] = NYT::TNode::CreateMap();
    if (BiometryClassification) {
        // https://st.yandex-team.ru/MEGAMIND-1682
        dialog["biometry_classification"] = NYT::NodeFromJsonString(JsonStringFromProto(*BiometryClassification));
    }
    dialog["biometry_scoring"] = NYT::TNode::CreateMap();
    if (BiometryScoring) {
        // https://st.yandex-team.ru/MEGAMIND-1682
        dialog["biometry_scoring"] = NYT::NodeFromJsonString(JsonStringFromProto(*BiometryScoring));
    }

    dialog["trash_or_empty_request"] = NYT::TNode::CreateEntity();
    if (TrashOrEmptyRequest) {
        dialog["trash_or_empty_request"] = *TrashOrEmptyRequest;
    }

    dialog["enrollment_headers"] = NYT::TNode::CreateEntity();
    if (EnrollmentHeaders) {
        dialog["enrollment_headers"] = NYT::NodeFromJsonString(JsonStringFromProto(*EnrollmentHeaders));
    }
    dialog["guest_data"] = NYT::TNode::CreateEntity();
    if (GuestData) {
        dialog["guest_data"] = NYT::NodeFromJsonString(JsonStringFromProto(*GuestData));
    }

    dialog["app_id"] = "pa";
    dialog["environment"] = "vins_stable";
    dialog["session_status"] = NYT::TNode::CreateEntity();
    dialog["provider"] = "megamind";
    dialog["error"] = NYT::TNode::CreateEntity();
    return dialog;
}

TDialogsBuilder::TErrors TDialogsBuilder::AddWonderlog(TWonderlog wonderlog) {
    TErrors errors;
    if (wonderlog.HasMessageId()) {
        MessageId_ = wonderlog.GetMessageId();
    }
    if (wonderlog.HasUuid()) {
        Uuid_ = wonderlog.GetUuid();
    }
    if (wonderlog.HasMegamindRequestId()) {
        MegamindRequestId_ = wonderlog.GetMegamindRequestId();
    }
    wonderlog.MutableSpeechkitRequest()->MutableRequest()->MutableAdditionalOptions()->SetDoNotUseUserLogs(
        wonderlog.GetPrivacy().GetDoNotUseUserLogs());

    if (wonderlog.GetSpotter().GetFalseActivation() || !wonderlog.HasSpeechkitRequest() ||
        !wonderlog.HasSpeechkitResponse()) {
        return errors;
    }

    for (const auto& directive : wonderlog.GetSpeechkitResponse().GetResponse().GetDirectives()) {
        if (directive.GetName() == "defer_apply") {
            return errors;
        }
    }

    // https://st.yandex-team.ru/MEGAMIND-2438
    if (ProductionEnvironment_) {
        const auto& environment = wonderlog.GetEnvironment();
        const TMaybe<TStringBuf> uniproxyQloudProject = environment.GetUniproxyEnvironment().HasQloudProject()
                                                            ? environment.GetUniproxyEnvironment().GetQloudProject()
                                                            : TMaybe<TStringBuf>{};
        const TMaybe<TStringBuf> uniproxyQloudApplication =
            environment.GetUniproxyEnvironment().HasQloudApplication()
                ? environment.GetUniproxyEnvironment().GetQloudApplication()
                : TMaybe<TStringBuf>{};
        const TMaybe<TStringBuf> megamindEnvironment = environment.GetMegamindEnvironment().HasEnvironment()
                                                           ? environment.GetMegamindEnvironment().GetEnvironment()
                                                           : TMaybe<TStringBuf>{};

        if (!ProductionEnvironment_->SuitableEnvironment(uniproxyQloudProject, uniproxyQloudApplication,
                                                         megamindEnvironment)) {
            return errors;
        }
    }

    try {
        Dialog_ = TDialog(wonderlog);
    } catch (const yexception& exc) {
        errors.push_back(GenerateError("R_FAILED_CREATE_DIALOG", TStringBuilder{}
                                                                     << exc.what() << " "
                                                                     << ToString(wonderlog.GetSpeechkitResponse())));
        return errors;
    }

    return errors;
}

bool TDialogsBuilder::Valid() const {
    return Dialog_.Defined();
}

NYT::TNode TDialogsBuilder::Build() && {
    return std::move(Dialog_)->DumpNode();
}

TDialogsBuilder::TError TDialogsBuilder::GenerateError(const TString& reason, const TString& message) {
    TError error;
    error["process"] = "P_DIALOGS_MAPPER";
    error["reason"] = reason;
    error["message"] = message;
    if (Uuid_) {
        error["uuid"] = *Uuid_;
    }
    if (MessageId_) {
        error["message_id"] = *MessageId_;
    }
    if (MegamindRequestId_) {
        error["megamind_request_id"] = *MegamindRequestId_;
    }
    if (auto setraceUrl = TryGenerateSetraceUrl({MessageId_, MegamindRequestId_, Uuid_})) {
        error["setrace_url"] = *setraceUrl;
    }
    return error;
}

} // namespace NAlice::NWonderlogs
