#include "request.h"

#include <alice/uniproxy/mapper/uniproxy_client/lib/async_uniproxy_client.h>

#include <alice/uniproxy/mapper/library/sensors/helpers.h>
#include <alice/library/json/json.h>

#include <library/cpp/json/json_reader.h>

#include <util/datetime/parser.h>
#include <util/generic/strbuf.h>
#include <util/stream/str.h>
#include <util/string/cast.h>

#include <iostream>

namespace NAlice::NUniproxy::NFetcher {
    using namespace NAlice::NUniproxy;
    using namespace NJson;

    namespace {
        using namespace NJson;

        std::unique_ptr<TUniproxyClient> CreateUniproxyClient(const TVoiceInput& request, const TString& uniproxyUrl,
                                                              const TString& defaultAuthToken, const TString& vinsUrl,
                                                              const TString& clientTime, i64 asrChunkSize, ui64 asrChunkDelayMs,
                                                              TLog* log, TFlagsContainer* flagsContainer,
                                                              TSensorContainer* sensors) {
            TUniproxyClientParams params;
            // TBaseClientParams
            params.Logger = log;
            params.FlagsContainer = flagsContainer;
            params.Sensors = sensors;
            auto headers = JsonFromString(TStringBuf(request.GetHeaders()));
            if (headers.IsMap()) {
                for (const auto& [header, value] : headers.GetMapSafe()) {
                    params.SessionHeaders[header] = value.GetStringSafe();
                }
            }
            // TUniproxyClientParams
            params.UniproxyUrl = uniproxyUrl;
            params.Uuid = request.GetUuid();
            params.AsrChunkSize = asrChunkSize;
            params.AsrChunkDelayMs = asrChunkDelayMs;
            params.DisableServerCertificateValidation = true;
            params.ApplicationId = request.GetApplicationId();
            params.ApplicationVersion = request.GetApplicationVersion();
            params.OsVersion = request.GetOsVersion();
            params.Platform = request.GetPlatform();
            params.Language = request.GetLang();
            params.Timezone = request.GetTimezone();
            params.VinsUrl = vinsUrl;
            params.ShootingSource = request.GetShootingSource();
            params.DisableLocalExperiments = request.GetDisableLocalExperiments();

            if (request.HasApplication()) {
                params.Application = JsonFromString(TStringBuf(request.GetApplication()));
                params.Application["timezone"] = request.GetTimezone();
                params.Application["client_time"] = clientTime;

                const auto tz = NDatetime::GetUtcTimeZone();
                std::chrono::system_clock::time_point tp;
                cctz::parse("%E4Y%m%dT%H%M%S", clientTime, tz, &tp);
                ui64 seconds = TInstant::MicroSeconds(tp.time_since_epoch().count()).Seconds();
                params.Application["timestamp"] = ToString(seconds);
            }

            if (request.HasOAuthToken()) {
                params.OAuthToken = request.GetOAuthToken();
            }
            auto const& rowAuthToken = request.GetAuthToken();
            params.AuthToken = rowAuthToken.empty() ? defaultAuthToken : rowAuthToken;

            if (request.HasSyncStateExperiments() && !request.GetSyncStateExperiments().empty()) {
                params.SyncStateExperiments = JsonFromString(TStringBuf(request.GetSyncStateExperiments()));
            }
            if (request.HasSupportedFeatures() && !request.GetSupportedFeatures().empty()) {
                params.SupportedFeatures = NAlice::JsonFromString(TStringBuf(request.GetSupportedFeatures()));
            }

            std::unique_ptr<TUniproxyClient> client;
            auto begin = TInstant::Now();

            if (flagsContainer && flagsContainer->Has("async_uniproxy_client")) {
                client = std::make_unique<TAsyncUniproxyClient>(params);
            } else {
                client = std::make_unique<TUniproxyClient>(params);
            }

            auto end = TInstant::Now();
            if (sensors != nullptr) {
                sensors->UniproxySyncStateTimeMs.Histogram->Record((end - begin).MilliSeconds());
            }
            return client;
        }

    }

    TUniproxyRequestPerformer::TUniproxyRequestPerformer(const TVoiceInput& request,
                                                         const TString& uniproxyUrl, const TString& defaultAuthToken,
                                                         const TString& vinsUrl, const TString& clientTime,
                                                         i64 asrChunkSize, ui64 asrChunkDelayMs,
                                                         TLog* log, TFlagsContainer* flagsContainer,
                                                         TStatisticsCallback statisticsCallback,
                                                         TSensorContainer* sensors)
        : StatisticsCallback{std::move(statisticsCallback)}
        , Client{CreateUniproxyClient(request, uniproxyUrl, defaultAuthToken, vinsUrl, clientTime,
                                      asrChunkSize, asrChunkDelayMs, log, flagsContainer, sensors)}
        , Sensors{sensors}
    {
    }

    void TUniproxyRequestPerformer::UpdateStatistics() {
        if (StatisticsCallback) {
            StatisticsCallback({Client->GetRemoteAddress(), Client->GetRemotePort()});
        }
    }

    TResponses TUniproxyRequestPerformer::VoiceRequest(const TString& requestId, const NJson::TJsonValue& payload,
                                                       const TString& topic, const TString& voiceData,
                                                       const TString& audioFormat) {
        auto begin = TInstant::Now();
        TExtraVoiceRequestParams requestParams;
        requestParams.RequestId = requestId;
        requestParams.PayloadTemplate = payload;
        requestParams.AudioFormat = audioFormat;
        UpdateStatistics();
        TStringInput voice(voiceData);
        auto responses = Client->SendVoiceRequest(topic, voice, requestParams, true, voiceData.length());

        auto end = TInstant::Now();
        if (Sensors != nullptr) {
            Sensors->UniproxyVoiceResponseTimeMs.Histogram->Record((end - begin).MilliSeconds());
            auto requestSize = payload.GetString().length();
            Sensors->UniproxyVoiceRequestPayloadSizeBytes.Histogram->Record(requestSize);
            auto voiceSize = voiceData.length();
            Sensors->UniproxyVoiceRequestVoiceSizeBytes.Histogram->Record(voiceSize);

            size_t responseSize = 0;
            for (const auto& response : responses) {
                responseSize += response.textSize();
            }
            Sensors->UniproxyVoiceResponsePayloadSizeBytes.Histogram->Record(responseSize);
            responseSize = 0;
            for (const auto& response : responses) {
                responseSize += response.voiceSize();
            }
            Sensors->UniproxyVoiceResponseVoiceSizeBytes.Histogram->Record(responseSize);
        }
        return responses;
    }

    TResponses TUniproxyRequestPerformer::TextRequest(const TString& requestId, const NJson::TJsonValue& payload,
                                                      const TString& textData) {
        auto begin = TInstant::Now();
        TExtraRequestParams requestParams;
        requestParams.RequestId = requestId;
        requestParams.PayloadTemplate = payload;
        UpdateStatistics();
        auto responses = Client->SendTextRequest(textData, requestParams);

        auto end = TInstant::Now();
        if (Sensors != nullptr) {
            Sensors->UniproxyTextResponseTimeMs.Histogram->Record((end - begin).MilliSeconds());
            auto requestSize = payload.GetString().length() + textData.length();
            Sensors->UniproxyTextRequestPayloadSizeBytes.Histogram->Record(requestSize);
            size_t responseSize = 0;
            for (const auto& response : responses) {
                responseSize += response.textSize();
            }
            Sensors->UniproxyTextResponsePayloadSizeBytes.Histogram->Record(responseSize);
        }
        return responses;
    }

    TResponses PerformUniproxyRequestInsideSession(TUniproxyRequestPerformer& performer,
                                                   const TVoiceInput& request) {
        auto requestId = request.GetRequestId();
        TJsonValue payload;
        ReadJsonTree(request.GetPayload(), &payload, /* throwOnErrors= */ true);
        if (request.HasFetcherMode() && request.GetFetcherMode() == "text") {
            // text mode
            return performer.TextRequest(requestId, payload, request.GetTextData());
        } else {
            // voice mode
            return performer.VoiceRequest(
                requestId, payload, request.GetTopic(), request.GetVoiceData(),
                request.GetFormat());
        }
    }

    TResponses PerformUniproxyRequest(const TVoiceInput& request, const TString& uniproxyUrl,
                                      const TString& defaultAuthToken, const TString& vinsUrl,
                                      const TString& clientTime, i64 asrChunkSize, ui64 asrChunkDelayMs,
                                      TLog* log, TFlagsContainer* flagsContainer,
                                      TStatisticsCallback statisticsCallback) {
        TUniproxyRequestPerformer performer(request, uniproxyUrl, defaultAuthToken, vinsUrl, clientTime,
                                            asrChunkSize, asrChunkDelayMs, log, flagsContainer, statisticsCallback);
        return PerformUniproxyRequestInsideSession(performer, request);
    }
}
