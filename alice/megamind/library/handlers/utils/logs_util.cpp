#include "logs_util.h"

#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <dj/lib/proto/action.pb.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/censor/lib/censor.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/metrics/names.h>
#include <alice/library/version/version.h>

namespace NAlice::NMegamind::NLogsUtil {

namespace {

inline constexpr TStringBuf OBFUSCATED_PLUG = "**OBFUSCATED**";

void ProcessLogs(IGlobalCtx& globalCtx, const NJson::TJsonValue& megamindAnalyticsLogsJson,
                 const NJson::TJsonValue& megamindProactivityLogsJson) {
    const auto megamindAnalyticsLogsString = ToString(megamindAnalyticsLogsJson);
    const auto megamindProactivityLogsString = ToString(megamindProactivityLogsJson);

    globalCtx.MegamindAnalyticsLog() << megamindAnalyticsLogsString << Endl;
    globalCtx.MegamindProactivityLog() << megamindProactivityLogsString << Endl;

    globalCtx.ServiceSensors().IncRate(NSignal::LOGGER_RECORDS_COUNT);
    globalCtx.ServiceSensors().AddRate(NSignal::LOGGER_RECORDS_LENGTH, megamindAnalyticsLogsString.Size());
}

void FilterSecretsFromRequest(NJson::TJsonValue& requestJson) {
    requestJson["additional_options"].EraseValue("oauth_token");
    auto& bassOptions = requestJson["additional_options"]["bass_options"];
    const auto& cookies = bassOptions["cookies"].GetArray();
    NJson::TJsonArray newCookies;
    for (const auto& cookie : cookies) {
        if (cookie.GetString().StartsWith("user_id=")) {
            newCookies.AppendValue(cookie);
            break;
        }
    }
    bassOptions.EraseValue("cookies");
    if (!newCookies.GetArray().empty()) {
        bassOptions["cookies"] = std::move(newCookies);
    }
}

constexpr TStringBuf IGNORE_ANSWER_PATH = "request/event/ignore_answer";
constexpr TStringBuf SESSION_PATH = "session";

bool ShouldObfuscateSession(const NJson::TJsonValue& json) {
    const NJson::TJsonValue* ignoreAnswerPtr = json.GetValueByPath(IGNORE_ANSWER_PATH, '/');
    return ignoreAnswerPtr != nullptr && ignoreAnswerPtr->GetBooleanSafe(false);
}

} // namespace

void PatchSpeechKitRequest(NJson::TJsonValue& skrJson) {
    skrJson.EraseValue("iot_user_info_data");
    skrJson["request"]["additional_options"].EraseValue("oauth_token");
}

NJson::TJsonValue CreateMegamindProactivityLogs(const NJson::TJsonValue& skrJson,
                                                const TMegamindAnalyticsInfo& analyticsInfo,
                                                const TProactivityLogStorage& proactivityLogStorage) {
    NJson::TJsonValue logs{NJson::JSON_MAP};

    logs["uuid"] = skrJson["application"]["uuid"];
    logs["device_id"] = skrJson["application"]["device_id"];
    logs["request_id"] = skrJson["header"]["request_id"];
    logs["sequence_number"] = skrJson["header"]["sequence_number"].GetInteger();
    logs["server_time"] = TInstant::Now().Seconds();
    logs["server_time_ms"] = TInstant::Now().MilliSeconds();
    logs["client_time"] = FromStringWithDefault<ui64>(skrJson["application"]["timestamp"].GetString(), 0);
    logs["client_tz"] = skrJson["application"]["timezone"];
    logs["app_info"] = skrJson["application"];

    logs["proactivity_info"] = JsonFromProto(analyticsInfo.GetModifiersInfo().GetProactivity());
    logs["proactivity_analytics"] = JsonFromProto(proactivityLogStorage.GetAnalytics());

    NJson::TJsonValue actions{NJson::JSON_ARRAY};
    for (const auto& actionProto : proactivityLogStorage.GetActions()) {
        NJson::TJsonValue actionJson;
        NProtobufJson::Proto2Json(actionProto, actionJson);
        actions.AppendValue(actionJson);
    }
    logs["actions"] = actions;

    const NJson::TJsonValue& biometryClassification = skrJson["event"]["biometry_classification"];
    if (biometryClassification.IsDefined()) {
        logs["biometry_classification"] = biometryClassification;
    } else {
        logs["biometry_classification"] = NJson::TJsonMap{};
    }

    return logs;
}

NJson::TJsonValue CreateMegamindAnalyticsLogs(const TConfig& config, NJson::TJsonValue requestJson,
                                              const NJson::TJsonValue& responseJson, bool containsSensitiveData,
                                              const TMegamindAnalyticsInfo& analyticsInfo,
                                              const TQualityStorage& qualityStorage,
                                              const bool dumpSessionsToAnalyticsLogs) {
    const auto& header = requestJson["header"];
    const auto& event = requestJson["event"];

    PatchSpeechKitRequest(requestJson);
    requestJson.EraseValue("contacts");
    FilterSecretsFromRequest(requestJson["request"]);

    auto modifiedResponseJson = responseJson;
    modifiedResponseJson.EraseValue(NAnalyticsInfo::MEGAMIND_ANALYTICS_INFO);
    if (modifiedResponseJson.Has("response")) {
        modifiedResponseJson["response"].EraseValue("quality_storage");
    }

    if (!dumpSessionsToAnalyticsLogs) {
        requestJson.EraseValue("session");
        modifiedResponseJson.EraseValue("sessions");
    }

    NJson::TJsonValue logs{NJson::JSON_MAP};

    logs["server_time_us"] = TInstant::Now().MicroSeconds();
    logs["uuid"] = requestJson["application"]["uuid"];
    logs["request_id"] = header["request_id"];
    logs["sequence_number"] = header["sequence_number"].GetIntegerSafe(0);
    logs["hypothesis_number"] = event["hypothesis_number"];

    logs["provider"] = TStringBuf("megamind");
    logs["environment"] = config.GetClusterType();
    logs["server_version"] = NAlice::VERSION_STRING;

    logs["contains_sensitive_data"] = containsSensitiveData;
    logs["content_type"] = NContentTypes::APPLICATION_JSON;
    logs["response"] = std::move(modifiedResponseJson);
    logs["analytics_info"] = JsonFromProto(analyticsInfo);
    logs["quality_storage"] = JsonFromProto(qualityStorage);

    logs["device_state"] = requestJson["request"]["device_state"];
    logs["additional_options"] = requestJson["request"]["additional_options"];
    logs["request"] = std::move(requestJson);
    return logs;
}

NJson::TJsonValue RemoveSensitiveData(const NJson::TJsonValue& original) {
    auto json = original;
    json["response"] = NJson::TJsonValue(NJson::JSON_NULL);
    json["sessions"] = NJson::TJsonValue(NJson::JSON_MAP);
    return json;
}

void PrepareAndProcessAnalyticsLogs(const NJson::TJsonValue& responseJson, bool hideSensitiveData,
                                    const NJson::TJsonValue& skrJson, const TMegamindAnalyticsInfo& analyticsInfo,
                                    const TQualityStorage& qualityStorage, IGlobalCtx& globalCtx,
                                    const TProactivityLogStorage& proactivityLogStorage,
                                    const bool dumpSessionsToAnalyticsLogs) {
    const auto& responseForLogs = hideSensitiveData ? RemoveSensitiveData(responseJson) : responseJson;
    const auto megamindAnalyticsLogs = CreateMegamindAnalyticsLogs(
        globalCtx.Config(), skrJson, responseForLogs, hideSensitiveData, analyticsInfo, qualityStorage, dumpSessionsToAnalyticsLogs);

    const auto megamindProactivityLogs =
        CreateMegamindProactivityLogs(skrJson, analyticsInfo, proactivityLogStorage);

    ProcessLogs(globalCtx, megamindAnalyticsLogs, megamindProactivityLogs);
}

bool ObfuscateBody(NJson::TJsonValue& json, TVector<TStringBuf> pathsToBeObfuscated) {
    if (json.IsNull()) {
        return false;
    }

    bool modified = false;
    if (ShouldObfuscateSession(json)) {
        pathsToBeObfuscated.push_back(SESSION_PATH);
    }
    for (const auto& path : pathsToBeObfuscated) {
        if (NJson::TJsonValue* tokenNode = json.GetValueByPath(path, '/')) {
            *tokenNode = OBFUSCATED_PLUG;
            modified = true;
        }
    }
    return modified;
}

void ObfuscateBody(TStringBuf original, NJson::TJsonValue& json, std::function<void(TStringBuf)> onObfusacatedBody,
                   const TVector<TStringBuf>& pathsToBeObfuscated) {
    try {
        if (ObfuscateBody(json, pathsToBeObfuscated)) {
            onObfusacatedBody(ToString(json));
        } else {
            onObfusacatedBody(original);
        }
    } catch (...) {
        onObfusacatedBody(original);
    }
}

// TFilterLogger ----------------------------------------------------------------------------------
TFilterLogger& TFilterLogger::SetFirstLine(const TString& firstLine) {
    FirstLine_ = firstLine;
    return *this;
}

TFilterLogger& TFilterLogger::SetContent(const TString& content) {
    Content_ = content;
    return *this;
}

TFilterLogger& TFilterLogger::SetContent(NJson::TJsonValue content, const TVector<TStringBuf>& pathsToBeObfuscated) {
    ObfuscateBody(content, pathsToBeObfuscated);
    return SetContent(ToString(content));
}

TFilterLogger& TFilterLogger::SetContent(TSpeechKitRequestProto&& content) {
    TCensor censor;
    censor.ProcessMessage(EAccess::A_PRIVATE_EVENTLOG, content);
    return SetContent(content.Utf8DebugString());
}

TFilterLogger& TFilterLogger::AddTag(TLogMessageTag&& tag) {
    Tags_.push_back(std::move(tag));
    return *this;
}

void TFilterLogger::Log(TRTLogger& logger) {
    LOG_INFO(logger) << FirstLine_;
    if (!Content_.Empty()) {
        LOG_INFO(logger) << Tags_ << Content_;
    }
}

} // namespace NAlice::NMegamind::NLogsUtil
