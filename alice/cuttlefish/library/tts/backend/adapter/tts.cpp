#include "tts.h"

using namespace NAlice::NTts;
using namespace NAlice::NTtsAdapter;

TTts::TTts(
    TIntrusivePtr<TCallbacks> callbacks,
    const TTtsClient::TConfig& ttsClientConfig,
    NVoicetech::THttpClient& httpClient,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : NTts::TInterface(callbacks)
    , Metrics_(TTts::SOURCE_NAME)
    , TtsClient_(
        ttsClientConfig,
        callbacks,
        httpClient,
        rtLogger
    )
{
}

void TTts::ProcessBackendRequest(const NProtobuf::TBackendRequest& backendRequest) {
    Metrics_.PushRate("backendrequest", "added");
    TtsClient_.AddBackendRequest(backendRequest);
}

void TTts::Cancel() {
    TtsClient_.CancelAll();
}
