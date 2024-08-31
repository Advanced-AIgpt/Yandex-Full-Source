#include "tts_generate.h"

#include "support_functions.h"

#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <util/system/byteorder.h>
#include <util/system/hostname.h>

#include <vector>

using namespace NAlice::NTts;
using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NJson;


namespace {

NJson::TJsonValue CreateTtsGeneratePayload(
    const NAliceProtocol::TRequestContext& requestContext,
    const TString& text,
    bool doNotLog
) {

    TJsonValue payload(JSON_MAP);
    TString mime;
    {
        auto& audioOptions = requestContext.GetAudioOptions();
        if (audioOptions.HasFormat()) {
            mime = audioOptions.GetFormat();
        }
    }
    if (mime) {
        payload["format"] = mime;
    } else {
        const NAliceProtocol::TAudioOptions& audioOptions = requestContext.GetAudioOptions();

        payload["format"] = audioOptions.HasFormat() ? audioOptions.GetFormat() : "Opus";
    }

    {
        const NAliceProtocol::TVoiceOptions& voiceOptions = requestContext.GetVoiceOptions();

        payload["lang"] = voiceOptions.HasLang() ? voiceOptions.GetLang() : "ru-RU";
        payload["voice"] = voiceOptions.HasVoice() ? voiceOptions.GetVoice() : "shitova.gpu";
        payload["quality"] = NTtsUtils::VoiceQualityToString(voiceOptions.GetQuality());
        if (voiceOptions.HasVolume()) {
            payload["volume"] = voiceOptions.GetVolume();
        }
        if (voiceOptions.HasSpeed()) {
            payload["speed"] = voiceOptions.GetSpeed();
        }
        payload["text"] = text;

        if (doNotLog) {
            payload["do_not_log"] = doNotLog;
        }
    }

    return payload;
}

}  // anonymous namespace

void NAlice::NCuttlefish::NAppHostServices::TtsGenerateMessageToTtsRequest(
    NAlice::NTts::NProtobuf::TRequest& ttsRequest,
    const NAliceProtocol::TRequestContext& requestContext,
    const NAliceProtocol::TSessionContext& sessionContext,
    const NVoicetech::NUniproxy2::TMessage& message
) {
    using namespace NVoicetech::NUniproxy2;

    const TMessage::THeader& header = NSupport::GetHeaderOrThrow(message);
    const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));

    {
        TString text;
        GetString(payload, TStringBuf("text"), &text);
        ttsRequest.SetText(text);
    }

    ttsRequest.SetPartialNumber(0);
    ttsRequest.SetRequestId(header.MessageId);

    // *shitova* -> shitova.gpu
    ttsRequest.SetReplaceShitovaWithShitovaGpu(NExpFlags::ExperimentFlagHasTrueValue(requestContext, "enable_tts_gpu"));

    // Enable tts backend timings by default to save them to cache
    // WARNING: Timings will not be sent to the client without NeedTtsBackendTimings
    ttsRequest.SetEnableTtsBackendTimings(true);

    // https://st.yandex-team.ru/VOICESERV-4209
    if (sessionContext.HasDeviceInfo()) {
        auto& deviceInfo = sessionContext.GetDeviceInfo();
        auto& supportedFeatures = deviceInfo.GetSupportedFeatures();
        for (const auto& feature : supportedFeatures) {
            if (feature == "enable_tts_timings") {
                ttsRequest.SetNeedTtsBackendTimings(true);
                break;
            }
        }
    }
    const auto& audioOptions = requestContext.GetAudioOptions();
    if (audioOptions.HasOpusRealTimeStreamer()) {
        ttsRequest.SetNeedRtsTimings(audioOptions.GetOpusRealTimeStreamer());
    }
    if (audioOptions.HasRtsBufferSeconds()) {
        ttsRequest.SetRtsBufferSeconds(audioOptions.GetRtsBufferSeconds());
    }

    if (NExpFlags::ExperimentFlagHasTrueValue(requestContext, "enable_tts_timings")) {
        // Send tts backend timings back only if they are requested
        ttsRequest.SetNeedTtsBackendTimings(true);
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(requestContext, "disable_tts_timings")) {
        // Totally disable timings
        // As in old python uniproxy https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8525222#L438-445
        // "disable_tts_timings" has higher priority than "enable_tts_timings"
        ttsRequest.SetNeedTtsBackendTimings(false);
        ttsRequest.SetEnableTtsBackendTimings(false);
    }

    {
        const bool cacheEnabled = (
            !requestContext.GetSettingsFromManager().HasTtsEnableCache() ||
            requestContext.GetSettingsFromManager().GetTtsEnableCache()
        );
        ttsRequest.SetEnableGetFromCache(cacheEnabled);
        ttsRequest.SetEnableCacheWarmUp(cacheEnabled);
        ttsRequest.SetEnableSaveToCache(cacheEnabled);
    }

    {
        bool doNotLog = false;
        if (GetBoolean(payload, TStringBuf("do_not_log"), &doNotLog)) {
            ttsRequest.SetDoNotLogTexts(doNotLog);
        }
    }
}

NVoicetech::NUniproxy2::TMessage NAlice::NCuttlefish::NAppHostServices::CreateTtsGenerate(
    const NAliceProtocol::TRequestContext& requestContext,
    const NAliceProtocol::TMegamindResponse& mmResponse,
    const TString& messageId
) {
    const TString& text = mmResponse.GetProtoResponse().GetVoiceResponse().GetOutputSpeech().GetText();
    const bool doNotLog = (
        mmResponse.GetProtoResponse().GetContainsSensitiveData() &&
        !NExpFlags::ExperimentFlagHasTrueValue(requestContext, "mm_black_sheep_wall")
    );
    Y_ENSURE(text);

    TJsonValue header(JSON_MAP);
    header["namespace"] = "TTS";
    header["name"] = "Generate";
    header["messageId"] = messageId;
    if (requestContext.GetHeader().HasRefStreamId()) {
        header["refStreamId"] = requestContext.GetHeader().GetRefStreamId();
    }

    TJsonValue root(JSON_MAP);
    root["event"]["header"] = std::move(header);
    root["event"]["payload"] = CreateTtsGeneratePayload(requestContext, text, doNotLog);

    return NVoicetech::NUniproxy2::TMessage(NVoicetech::NUniproxy2::TMessage::FromAppHost, std::move(root));
}
