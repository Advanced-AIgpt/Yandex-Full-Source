#pragma once

#include <alice/uniproxy/mapper/fetcher/lib/protos/voice_input.pb.h>
#include <alice/uniproxy/mapper/uniproxy_client/lib/uniproxy_client.h>

#include <alice/uniproxy/mapper/library/flags/container.h>
#include <alice/uniproxy/mapper/library/sensors/constants.h>

#include <library/cpp/logger/log.h>
#include <library/cpp/retry/retry.h>

#include <functional>

namespace NAlice::NUniproxy::NFetcher {
    struct TAddressAndPort {
        TString IPAddress;
        ui16 Port = 0;
    };

    using TStatisticsCallback = std::function<void(const TAddressAndPort&)>;

    class TUniproxyRequestPerformer {
    private:
        TStatisticsCallback StatisticsCallback;
        std::unique_ptr<TUniproxyClient> Client;
        NAlice::NUniproxy::TSensorContainer* Sensors;

    public:
        TUniproxyRequestPerformer(const TVoiceInput& request, const TString& uniproxyUrl,
                                  const TString& authToken, const TString& vinsUrl,
                                  const TString& clientTime,
                                  i64 asrChunkSize, ui64 asrChunkDelayMs,
                                  TLog* log = nullptr, TFlagsContainer* flagsContainer = nullptr,
                                  TStatisticsCallback statisticsCallback = {},
                                  NAlice::NUniproxy::TSensorContainer* sensors = nullptr);

        NAlice::NUniproxy::TResponses VoiceRequest(const TString& requestId, const NJson::TJsonValue& payload,
                                                   const TString& topic, const TString& voiceData,
                                                   const TString& audioFormat);
        NAlice::NUniproxy::TResponses TextRequest(const TString& requestId, const NJson::TJsonValue& payload,
                                                  const TString& textData);

    private:
        void UpdateStatistics();
    };

    NAlice::NUniproxy::TResponses PerformUniproxyRequest(const TVoiceInput& request, const TString& uniproxyUrl,
                                                         const TString& defaultAuthToken, const TString& vinsUrl,
                                                         const TString& clientTime,
                                                         i64 asrChunkSize, ui64 asrChunkDelayMs, TLog* log = nullptr,
                                                         TFlagsContainer* flagsContainer = nullptr,
                                                         TStatisticsCallback statisticsCallback = {});

    NAlice::NUniproxy::TResponses PerformUniproxyRequestInsideSession(TUniproxyRequestPerformer& performer,
                                                                      const TVoiceInput& request);
}
