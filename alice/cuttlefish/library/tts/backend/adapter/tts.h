#pragma once

#include <alice/cuttlefish/library/tts/backend/base/metrics.h>
#include "tts_client.h"

namespace NAlice::NTtsAdapter {
    class TTts : public NTts::TInterface {
    public:
        TTts(
            TIntrusivePtr<TCallbacks> callbacks,
            const TTtsClient::TConfig& ttsClientConfig,
            NVoicetech::THttpClient& httpClient,
            NRTLog::TRequestLoggerPtr rtLogger
        );

        void ProcessBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest) override;
        void Cancel() override;

    protected:
        static constexpr TStringBuf SOURCE_NAME = "tts";
        NTts::TSourceMetrics Metrics_;

        TTtsClient TtsClient_;
    };
}
