#pragma once

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/http/server/response.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NMegamind::NLogsUtil {

inline const TVector<TStringBuf> BASE_PATHS_TO_BE_OBFUSCATED_IN_REQUEST = {
    "contacts/data",
    "contacts_proto",
    "request/additional_options/bass_options/cookies",
    "request/additional_options/guest_user_options/oauth_token",
    "request/additional_options/oauth_token",
};

void PatchSpeechKitRequest(NJson::TJsonValue& skrJson);

NJson::TJsonValue CreateMegamindAnalyticsLogs(const TConfig& config, const NJson::TJsonValue requestJson,
                                              const NJson::TJsonValue& responseJson, bool containsSensitiveData,
                                              const TMegamindAnalyticsInfo& analyticsInfo,
                                              const TQualityStorage& qualityStorage,
                                              const bool dumpSessionsToAnalyticsLogs);

NJson::TJsonValue CreateMegamindProactivityLogs(const NJson::TJsonValue& skrJson,
                                                const TMegamindAnalyticsInfo& analyticsInfo,
                                                const TProactivityLogStorage& proactivityLogStorage);

// In accordance with MEGAMIND-16
// XXX this function is used only in alice/logs/wonderlogs/ and should be moved somewhere outside
// megamind dir
NJson::TJsonValue CreateVinsLikeLogMessage(const NJson::TJsonValue& responseJson,
                                           const TMegamindAnalyticsInfo& analyticsInfo,
                                           const TSpeechKitRequestProto& skr, const TEvent& event,
                                           const NJson::TJsonValue& vinsLikeRequest);

NJson::TJsonValue RemoveSensitiveData(const NJson::TJsonValue& original);

void PrepareAndProcessAnalyticsLogs(const NJson::TJsonValue& responseJson, bool hideSensitiveData,
                                    const NJson::TJsonValue& skrJson, const TMegamindAnalyticsInfo& analyticsInfo,
                                    const TQualityStorage& qualityStorage, IGlobalCtx& globalCtx,
                                    const TProactivityLogStorage& proactivityLogStorage,
                                    const bool dumpSessionsToAnalyticsLogs);

void ObfuscateBody(TStringBuf original, NJson::TJsonValue& json, std::function<void(TStringBuf)> onObfusacatedBody,
                   const TVector<TStringBuf>& pathsToBeObfuscated);

bool ObfuscateBody(NJson::TJsonValue& json, TVector<TStringBuf> pathsToBeObfuscated);

class TFilterLogger {
public:
    TFilterLogger& SetFirstLine(const TString& firstLine);

    TFilterLogger& SetContent(TSpeechKitRequestProto&& content);
    TFilterLogger& SetContent(NJson::TJsonValue content, const TVector<TStringBuf>& pathsToBeObfuscated);
    TFilterLogger& SetContent(const TString& content);

    TFilterLogger& AddTag(TLogMessageTag&& tag);

    void Log(TRTLogger& logger);

private:
    TString FirstLine_;
    TString Content_;
    TVector<TLogMessageTag> Tags_;
};

} // namespace NAlice::NMegamind::NLogsUtil
