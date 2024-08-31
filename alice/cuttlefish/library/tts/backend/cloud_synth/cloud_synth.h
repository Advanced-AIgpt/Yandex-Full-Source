#pragma once

#include <alice/cuttlefish/library/tts/backend/base/interface.h>
#include <alice/cuttlefish/library/tts/backend/base/metrics.h>

#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <util/generic/hash.h>

namespace NAlice::NCloudSynth {
    class TCloudSynth : public NTts::TInterface {
    public:
        struct TConfig {
            TString BackendUrl_;
            TString LegacyOauthToken_;
            bool Insecure_;
            THashMap<TString, TString> VoiceToToken_;
        };

    public:
        TCloudSynth(
            TIntrusivePtr<TCallbacks> callbacks,
            const TConfig& config,
            NRTLog::TRequestLoggerPtr rtLogger
        );

        void ProcessBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest) override;
        void Cancel() override;

    private:
        static constexpr TStringBuf SOURCE_NAME = "cloud_synth";
        NTts::TSourceMetrics Metrics_;

        const TConfig& Config_;
        NRTLog::TRequestLoggerPtr RtLogger_;
        bool Cancelled_ = false;
    };
}
