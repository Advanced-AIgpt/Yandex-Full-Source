#include "ws_stream_to_proto.h"

#include "biometry.h"
#include "megamind.h"
#include "music_match_request.h"
#include "smart_activation.h"
#include "support_functions.h"
#include "tts_generate.h"

#include <alice/cuttlefish/library/cuttlefish/context_load/client/starter.h>
#include <alice/cuttlefish/library/cuttlefish/stream_converter/rms_converter/converter.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>
#include <alice/cuttlefish/library/experiments/patch_functions.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/proto_censor/megamind.h>
#include <alice/cuttlefish/library/proto_censor/session_context.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>
#include <alice/cuttlefish/library/protos/bio_context_save.pb.h>

#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/library/json/json.h>
#include <google/protobuf/timestamp.pb.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/guid.h>


using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NAppHostServices::NSupport;
using namespace NJson;
using TMessage = NVoicetech::NUniproxy2::TMessage;

namespace {
    class TInvalidAsrParameters: public yexception {
    };

    void CheckAsrBackend(const TString& mark) {
        static const THashSet<TString> allowedRoutes = {
            "asr_ru-ru_freeform",
            "asr_ru-ru_dialog-maps-gpu",
            "asr_ru-ru_dialog-general-gpu",
            "asr_ru-ru_quasar-general-gpu",
            "asr_ru-ru_tv-general-gpu",
            "asr_tr-tr_dialog-maps-gpu",
            "asr_ar-sa_quasar-general-gpu",
        };
        if (!allowedRoutes.contains(mark)) {
            throw TInvalidAsrParameters() << "backend " << mark << " not allowed for usage";
        }
    }

    const TString WS_ADAPTER_IN = "WS_ADAPTER_IN";
}


TWsStreamToProtobuf::TWsStreamToProtobuf(
    NAppHost::TServiceContextPtr ctx,
    const NVoice::NExperiments::TExperiments& exp,
    TLogContext logContext
)
    : AhContext(ctx)
    , Promise(NThreading::NewPromise())
    , LogContext(logContext)
    , Metrics(*ctx, "ws_adapter_in")
    , Experiments(exp)
    , MatchedUserEventHandler(ctx, logContext, Metrics)
{
    LogContext.LogEventInfoCombo<NEvClass::WsStreamToProto>(GetGuidAsString(ctx->GetRequestID()));
}

void TWsStreamToProtobuf::OnNextInput() {
    using namespace NAliceProtocol;
    static const TVector<TStringBuf> processingFirst = {
        ITEM_TYPE_SETTINGS,
        ITEM_TYPE_SESSION_CONTEXT,
    };
    for (auto& type : processingFirst) {
        auto items = AhContext->GetProtobufItemRefs(type, NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (!OnAppHostProtoItem(it.GetType(), *it)) {
                OnBreakProcessing();
                Promise.SetValue();
                return;
            }
        }
    }

    auto items = AhContext->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (std::count(processingFirst.begin(), processingFirst.end(), it.GetType())) {
            continue;  // this type already processed
        }

        if (!OnAppHostProtoItem(it.GetType(), *it)) {
            OnBreakProcessing();
            Promise.SetValue();
            return;
        }
    }

    if (NeedFlush) {
        AhContext->IntermediateFlush();
        NeedFlush = false;
    }
    // continue consume input
    AhContext->NextInput().Apply([self = TIntrusivePtr<TWsStreamToProtobuf>(this)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            self->OnEndInput();
            self->Promise.SetValue();
            return;
        }

        self->OnNextInput();
    });
}

bool TWsStreamToProtobuf::OnAppHostProtoItem(TStringBuf type, const NAppHost::NService::TProtobufItem& item) {
    using namespace NAliceProtocol;

    if (type == ITEM_TYPE_SESSION_CONTEXT) {
        try {
            ParseProtobufItem(item, SessionContext);
            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostSessionContext>(
                CensoredSessionContextStr(SessionContext)
            );
            HasSessionContext = true;
            return true;
        } catch (...) {
            AddEventException("bad_session_context", "TSessionContext parse protobuf error", CurrentExceptionMessage());
            return false;
        }
    }
    if (type == ITEM_TYPE_SETTINGS) {
        try {
            ParseProtobufItem(item, SettingsFromManager);
            HasManagerSettings = true;
            return true;
        } catch (...) {
            AddEventException("bad_settings_from_manager", "TManagedSettings parse protobuf error", CurrentExceptionMessage());
            return false;
        }
    }

    if (type != ITEM_TYPE_WS_MESSAGE) {
        return true;
    }

    TWsEvent wsEvent;
    try {
        ParseProtobufItem(item, wsEvent);
    } catch (...) {
        AddEventException("bad_wsevent", "TWsStreamToProtobuf parse protobuf error", CurrentExceptionMessage());
        return false;
    }

    if (wsEvent.HasBinary()) {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostWsEventBinary>(wsEvent.GetBinary().Size());
    } else {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    }

    if (wsEvent.HasHeader() && wsEvent.HasText()) {
        const TEventHeader& header = wsEvent.GetHeader();
        const TEventHeader::EMessageName msgName = header.GetName();

        if (!RefMessageId && header.HasMessageId()) {
            RefMessageId = header.GetMessageId();
        }
        try {
            if ((header.GetNamespace() == TEventHeader::ASR && header.GetName() == TEventHeader::RECOGNIZE)) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnAsrRecognize(message);
            } else if ((header.GetNamespace() == TEventHeader::TTS && header.GetName() == TEventHeader::GENERATE)) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                if (!OnTtsGenerate(message)) {
                    return false;
                }
            } else if (header.GetNamespace() == TEventHeader::VINS) {
                if (!EqualToOneOf(msgName, TEventHeader::VOICE_INPUT, TEventHeader::TEXT_INPUT, TEventHeader::MUSIC_INPUT))
                    return true;  // ignore unknown messages

                const TMessage message = CreateMessagePatchedWithExperiments(TMessage::FromClient, wsEvent.GetText());
                OnVinsRequest(message);
                if (msgName == TEventHeader::VOICE_INPUT) {
                    OnVinsVoiceInput(message);
                } else if (msgName == TEventHeader::TEXT_INPUT) {
                    OnVinsTextInput(message);
                } else if (msgName == TEventHeader::MUSIC_INPUT) {
                    OnVinsMusicInput(message);
                }
            } else if (header.GetNamespace() == TEventHeader::BIOMETRY && header.GetName() == TEventHeader::CLASSIFY) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnBiometryClassify(message);
            } else if (header.GetNamespace() == TEventHeader::BIOMETRY && header.GetName() == TEventHeader::IDENTIFY) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnBiometryIdentify(message);
            } else if (header.GetNamespace() == TEventHeader::BIOMETRY && header.GetName() == TEventHeader::CREATE_OR_UPDATE_USER) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnBiometryCreateOrUpdateUser(message);
            } else if (header.GetNamespace() == TEventHeader::BIOMETRY && header.GetName() == TEventHeader::GET_USERS) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnBiometryGetUsers(message);
            } else if (header.GetNamespace() == TEventHeader::BIOMETRY && header.GetName() == TEventHeader::REMOVE_USER) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnBiometryRemoveUser(message);
            } else if (header.GetNamespace() == TEventHeader::LOG && header.GetName() == TEventHeader::NM_SPOTTER) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                OnLogSpotter(message);
            } else if (header.GetNamespace() == TEventHeader::SYSTEM && header.GetName() == TEventHeader::MATCHED_USER) {
                TMessage message(TMessage::FromClient, wsEvent.GetText());
                MatchedUserEventHandler.OnEvent(message, SessionContext.GetConnectionInfo().GetIpAddress());
            } else if (header.GetNamespace() == TEventHeader::STREAM_CONTROL) {
                OnStreamControl(header);
            } else {
                // ignore unknown messages
            }
        } catch (const NJson::TJsonException& exc) {
            AddEventException("bad_json", "Incorrect JSON", exc.what());
            return false;
        } catch (const TInvalidAsrParameters& exc) {
            AddEventException("bad_asr_request", TStringBuilder() << "Invalid ASR parameters: " << exc.what());
            return false;
        } catch (const TMegamindRequestConstructorError&) {
            AddEventException("bad_mm_request", TStringBuilder() << "Megamind json parser failed", CurrentExceptionMessage());
        } catch (...) {
            AddEventException("unexpected_excepion", "unknown exception in WsStreamToProtobuf::OnAppHostProtoItem", CurrentExceptionMessage());
            return false;
        }
    } else if (wsEvent.HasBinary()) {
        // if has binary message build TAudio chunk
        if (StreamStatus == ProcessSpotter || StreamStatus == ProcessStream) {
            NAliceProtocol::TAudio audioChunk;
            if (wsEvent.GetBinary().Size() >= 4) {
                // skip stream_id from chunk
                audioChunk.MutableChunk()->SetData(wsEvent.GetBinary().data() + 4, wsEvent.GetBinary().Size() - 4);

                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudioChunk>(audioChunk.GetChunk().GetData().Size(), "" /* meta info is empty */);
                AhContext->AddProtobufItem(audioChunk, ITEM_TYPE_AUDIO);
                NeedFlush = true;
            } // else ignore strange chunk
        } else {
            AddEventException("unexpected_audio_message", "got binary ws message (audio) outside started stream");
            return false;
        }
    }
    return true;
}

void TWsStreamToProtobuf::OnEndInput() {
    //TODO:? send SpotterEnd/StreamEnd if need ?
    LogContext.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
}

void TWsStreamToProtobuf::OnBreakProcessing() {
    //TODO:?
}

void TWsStreamToProtobuf::OnAsrRecognize(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    if (IsMusicMatchRequestViaAsr(message)) {
        if (!IsRecognizeMusicOnly(message)) {
            StartAudio(message, (TAudioFlags::ASR | TAudioFlags::MUSIC_MATCH));
        } else {
            StartAudio(message, TAudioFlags::MUSIC_MATCH);
        }
    } else {
        StartAudio(message, TAudioFlags::ASR);
    }
}

bool TWsStreamToProtobuf::OnTtsGenerate(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    try {
        NAlice::NTts::NProtobuf::TRequest ttsRequest;
        TtsGenerateMessageToTtsRequest(ttsRequest, RequestContext, SessionContext, message);
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostTtsRequest>(GetCensoredTtsRequestStr(ttsRequest), /* isFinal = */ true);
        AhContext->AddProtobufItem(ttsRequest, ITEM_TYPE_TTS_REQUEST);
        // Flag for any input graph edge expressions
        AhContext->AddFlag(EDGE_FLAG_TTS);
        NeedFlush = true;
    } catch (...) {
        AddEventException("tts_generate_handle_exception", "TTS.Generate handling error", CurrentExceptionMessage());
        return false;
    }

    return true;
}

void TWsStreamToProtobuf::OnVinsRequest(const TMessage& message) {
    BuildRequestContext(message);
    NAliceProtocol::TMegamindRequest mmRequest;
    MessageToMegamindRequest(mmRequest, RequestContext, message, SessionContext, &LogContext);
    AhContext->AddProtobufItem(mmRequest, ITEM_TYPE_MEGAMIND_REQUEST);
    Censore(mmRequest);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMegamindRequest>(mmRequest.ShortUtf8DebugString());

    if (mmRequest.GetRequestBase().HasEnrollmentHeaders()) {
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostEnrollmentHeaders>(mmRequest.GetRequestBase().GetEnrollmentHeaders());
        AhContext->AddProtobufItem(mmRequest.GetRequestBase().GetEnrollmentHeaders(), ITEM_TYPE_ENROLLMENT_HEADERS);
    }

    // Loading guest context for TextInput
    if (mmRequest.GetRequestBase().HasRequest() &&
        mmRequest.GetRequestBase().GetRequest().HasAdditionalOptions() &&
        mmRequest.GetRequestBase().GetRequest().GetAdditionalOptions().HasGuestUserOptions() &&
        mmRequest.GetRequestBase().GetRequest().GetAdditionalOptions().GetGuestUserOptions().GetOAuthToken()
    ) {
        Metrics.PushRate("request", "oauth_token", "blackbox");
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxHttpRequest>("oauth_token");

        NAppHostHttp::THttpRequest blackboxRequest = TBlackboxClient::GetUidForOAuth(
            mmRequest.GetRequestBase().GetRequest().GetAdditionalOptions().GetGuestUserOptions().GetOAuthToken(),
            SessionContext.GetConnectionInfo().GetIpAddress());

        AhContext->AddProtobufItem(blackboxRequest, ITEM_TYPE_GUEST_BLACKBOX_HTTP_REQUEST);
    }

    NeedFlush = true;
}

void TWsStreamToProtobuf::OnVinsVoiceInput(const TMessage& message) {
    StartContextLoad(message);

    if (RequestContext.GetPredefinedResults().GetAsr()) {
        if (const auto* result = message.Json.GetValueByPath("event.payload.request.predefined_asr_result")) {
            NAliceProtocol::TPredefinedAsrResult protoResult;
            protoResult.SetPayload(NJson::WriteJson(result, /*formatOutput=*/ false, /*sortkeys=*/ true));
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostPredefinedAsrResult>(protoResult.ShortUtf8DebugString());
            AhContext->AddProtobufItem(protoResult, ITEM_TYPE_PREDEFINED_ASR_RESULT);
        }
        return;
    }

    TAudioFlags audioFlags(TAudioFlags::ASR);
    if (RequestContext.HasBiometryOptions()) {
        auto& bioOpt = RequestContext.GetBiometryOptions();
        if (!RequestContext.GetPredefinedResults().GetBioClassify() && bioOpt.HasClassify() && bioOpt.GetClassify().size()) {
            audioFlags |= TAudioFlags::YABIO_CLASSIFY;
        }
        if (!RequestContext.GetPredefinedResults().GetBioScoring() && bioOpt.HasScore() && bioOpt.GetScore() && bioOpt.HasSendScoreToMM() && bioOpt.GetSendScoreToMM()) {
            audioFlags |= TAudioFlags::YABIO_SCORE;
        }
    }
    {
        // voice_input graph required item "activation_success" (for use in smart_activation disabled mode, or as stub on smart_activation errors)
        NCachalotProtocol::TActivationSuccessful success;
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostActivationSuccessful>("-");
        AhContext->AddProtobufItem(success, ITEM_TYPE_ACTIVATION_SUCCESSFUL);
    }
    StartAudio(message, audioFlags);
    AddRequestContext();
}

void TWsStreamToProtobuf::OnVinsTextInput(const TMessage& message) {
    {
        // voice_input graph required item "activation_success" (for use in smart_activation disabled mode, or as stub on smart_activation errors)
        NCachalotProtocol::TActivationSuccessful success;
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostActivationSuccessful>("-");
        AhContext->AddProtobufItem(success, ITEM_TYPE_ACTIVATION_SUCCESSFUL);
    }
    AddRequestContext();
    StartContextLoad(message);
    NeedFlush = true;
}

void TWsStreamToProtobuf::OnVinsMusicInput(const TMessage& message) {
    AddRequestContext();
    StartContextLoad(message);
    StartAudio(message, TAudioFlags::MUSIC_MATCH);
}

void TWsStreamToProtobuf::OnLogSpotter(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StreamStatus = ProcessSpotter;
}

void TWsStreamToProtobuf::OnBiometryClassify(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StartAudio(message, TAudioFlags::YABIO_CLASSIFY);
}

void TWsStreamToProtobuf::OnBiometryIdentify(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StartAudio(message, TAudioFlags::YABIO_SCORE);
}

void TWsStreamToProtobuf::OnBiometryCreateOrUpdateUser(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StartCreateOrUpdateBioContextUser(message);
}

void TWsStreamToProtobuf::OnBiometryGetUsers(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StartLoadBiometryContext(message);
}

void TWsStreamToProtobuf::OnBiometryRemoveUser(const TMessage& message) {
    if (BuildRequestContext(message)) {
        AddRequestContext();
    }
    StartRemoveBiometryContextUser(message);
}

void TWsStreamToProtobuf::OnStreamControl(const NAliceProtocol::TEventHeader& header) {
    // TODO: check streamId ?
    if (header.HasAction()) {
        if (header.GetAction() == 0) { // EOS
            if (StreamStatus != StreamEnded) {
                StreamStatus = StreamEnded;
                NAliceProtocol::TAudio audio;
                audio.MutableEndStream();

                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audio.ShortUtf8DebugString());
                AhContext->AddProtobufItem(audio, ITEM_TYPE_AUDIO);
                NeedFlush = true;
            }
        } else if (header.GetAction() == 2) { // End Of Spotter
            if (StreamStatus == ProcessSpotter) {
                StreamStatus = ProcessStream;
                NAliceProtocol::TAudio audio;
                audio.MutableEndSpotter();

                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audio.ShortUtf8DebugString());
                AhContext->AddProtobufItem(audio, ITEM_TYPE_AUDIO);
                NeedFlush = true;
            }
        }
    }
}

void TWsStreamToProtobuf::ProcessAsrInitData(const NAlice::NAsr::NProtobuf::TInitRequest& initRequest) {
    TStringStream ss;
    ss << TStringBuf("asr_") << to_lower(initRequest.GetLang());
    AhContext->AddFlag(ss.Str()); // metaflag for route to asr_fallback node

    {
        TString topic = initRequest.GetTopic();
        // crutch for similar topic naming (dialog-general-gpu, qusasar-generar-gpu, ...)
        if (topic == "dialogmapsgpu") {
            topic = "dialog-maps-gpu";
        }
        if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "asr_quasar_monolith")) {  // exp name from https://st.yandex-team.ru/VOICE-6259#60674f882c0fea68e1bcbe02
            topic = "quasar-general-gpu-monolith";
        }
        if (topic == "quasar-general-monolith") {  // hack for VOICE-6259
            topic = "quasar-general-gpu-monolith";
        }
        ss << TStringBuf("_") << topic;
    }
    CheckAsrBackend(ss.Str());  // throw exception on unsupported lang+topic pair (check requested by @avitella)
    AhContext->AddFlag(ss.Str()); // metaflag for route to main asr node

    // VOICESERV-4092: Responses from shadow asrs are discarded, its ok to run multiple of them
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "asr-shadow-gpu")) {
        AhContext->AddFlag("asr-shadow-gpu");
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "asr-shadow-cpu")) {
        AhContext->AddFlag("asr-shadow-cpu");
    }

    // Flag for any input graph edge expressions
    AhContext->AddFlag(EDGE_FLAG_ASR);
}

void TWsStreamToProtobuf::ProcessMusicMatchInitData(const NAlice::NMusicMatch::NProtobuf::TInitRequest& initRequest) {
    Y_UNUSED(initRequest);
    // Flag for any input graph edge expressions
    AhContext->AddFlag(EDGE_FLAG_MUSIC_MATCH);
}

void TWsStreamToProtobuf::ProcessYabioInitClassifyData(const YabioProtobuf::YabioRequest& initRequest) {
    Y_UNUSED(initRequest);
    AhContext->AddFlag(EDGE_FLAG_BIO_CLASSIFY);
}

void TWsStreamToProtobuf::ProcessYabioInitScoreData(const YabioProtobuf::YabioRequest& initRequest) {
    Y_UNUSED(initRequest);
    AhContext->AddFlag(EDGE_FLAG_LOAD_BIO_CONTEXT);
    AhContext->AddFlag(EDGE_FLAG_BIO_SCORE);
}

void TWsStreamToProtobuf::StartAudio(const TMessage& message, TAudioFlags audioFlags) {
    if (audioFlags.HasYabioClassify()) {
        NAliceProtocol::TAudio audioYabio;
        audioYabio.MutableMetaInfoOnly();
        auto& yabioInitRequest = *audioYabio.MutableYabioInitRequest();
        MessageToYabioInitRequestClassify(message, yabioInitRequest, SessionContext, RequestContext, &LogContext);
        if (auto header = message.Header.Get()) {
            if (header->Class == NVoicetech::NUniproxy2::EMessageClass::BIOMETRY_CLASSIFY) {
                // support python_uniproxy Biometery.Classify compatibility (disable bio partials)
                yabioInitRequest.SetOverridePartialUpdatePeriod(100000000);
            }
        }
        ProcessYabioInitClassifyData(yabioInitRequest);

        SendStartStream(yabioInitRequest.mime(), yabioInitRequest.has_spotter() && yabioInitRequest.spotter());
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioYabio.ShortUtf8DebugString());
        AhContext->AddProtobufItem(audioYabio, ITEM_TYPE_AUDIO);
        audioFlags.Reset(TAudioFlags::YABIO_CLASSIFY);
        NeedFlush = true;
        if (!audioFlags.GetBits()) {
            return;
        }
    }
    if (audioFlags.HasYabioScore()) {
        StartLoadBiometryContext(message);

        NAliceProtocol::TAudio audioYabio;
        audioYabio.MutableMetaInfoOnly();
        auto& yabioInitRequest = *audioYabio.MutableYabioInitRequest();
        MessageToYabioInitRequestScore(message, yabioInitRequest, SessionContext, RequestContext, &LogContext);
        if (auto header = message.Header.Get()) {
            if (header->Class == NVoicetech::NUniproxy2::EMessageClass::BIOMETRY_IDENTIFY) {
                // support python_uniproxy Biometery.Identify compatibility (disable bio partials)
                yabioInitRequest.SetOverridePartialUpdatePeriod(100000000);
            }
        }
        ProcessYabioInitScoreData(yabioInitRequest);
        SendStartStream(yabioInitRequest.mime(), yabioInitRequest.has_spotter() && yabioInitRequest.spotter());
        //AhContext->AddFlag(EDGE_FLAG_LOAD_BIO_CONTEXT);
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioYabio.ShortUtf8DebugString());
        AhContext->AddProtobufItem(audioYabio, ITEM_TYPE_AUDIO);
        audioFlags.Reset(TAudioFlags::YABIO_SCORE);
        NeedFlush = true;
        if (!audioFlags.GetBits()) {
            return;
        }
    }
    switch (audioFlags.GetBits()) {
        case (TAudioFlags::ASR | TAudioFlags::MUSIC_MATCH): {
            // Special legacy case
            // Just copy this logic: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/asr.py?rev=r7818350#L155-183

            NAliceProtocol::TAudio audioAsr;
            audioAsr.MutableMetaInfoOnly();
            auto& asrInitRequest = *audioAsr.MutableAsrInitRequest();
            MessageToAsrInitRequest(message, asrInitRequest, SessionContext, RequestContext, &LogContext);

            NAliceProtocol::TAudio audioMusicMatch;
            audioMusicMatch.MutableMetaInfoOnly();
            auto& musicMatchInitRequest = *audioMusicMatch.MutableMusicMatchInitRequest();
            AsrMessageToMusicMatchInitRequest(message, musicMatchInitRequest);

            ProcessAsrInitData(asrInitRequest);
            ProcessMusicMatchInitData(musicMatchInitRequest);
            // Use mime from asr
            // Music match uses its own way of recognizing the audio type
            SendStartStream(asrInitRequest.GetMime(), asrInitRequest.HasHasSpotterPart() && asrInitRequest.GetHasSpotterPart());

            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioAsr.ShortUtf8DebugString());
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioMusicMatch.ShortUtf8DebugString());

            AhContext->AddProtobufItem(audioAsr, ITEM_TYPE_AUDIO);
            AhContext->AddProtobufItem(audioMusicMatch, ITEM_TYPE_AUDIO);

            break;
        }
        case TAudioFlags::ASR: {
            NAliceProtocol::TAudio audioAsr;
            audioAsr.MutableMetaInfoOnly();
            auto& asrInitRequest = *audioAsr.MutableAsrInitRequest();
            MessageToAsrInitRequest(message, asrInitRequest, SessionContext, RequestContext, &LogContext);

            ProcessAsrInitData(asrInitRequest);
            SendStartStream(asrInitRequest.GetMime(), asrInitRequest.HasHasSpotterPart() && asrInitRequest.GetHasSpotterPart());

            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioAsr.ShortUtf8DebugString());
            AhContext->AddProtobufItem(audioAsr, ITEM_TYPE_AUDIO);

            break;
        }
        case TAudioFlags::MUSIC_MATCH: {
            NAliceProtocol::TAudio audioMusicMatch;
            audioMusicMatch.MutableMetaInfoOnly();
            auto& musicMatchInitRequest = *audioMusicMatch.MutableMusicMatchInitRequest();
            VinsMessageToMusicMatchInitRequest(message, musicMatchInitRequest);

            ProcessMusicMatchInitData(musicMatchInitRequest);
            SendStartStream(musicMatchInitRequest.GetAudioFormat());

            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audioMusicMatch.ShortUtf8DebugString());
            AhContext->AddProtobufItem(audioMusicMatch, ITEM_TYPE_AUDIO);

            break;
        }
        default:
            ythrow yexception() << "Can't start audio stream with " << audioFlags.GetBits() << " audio flags";
    }
    NeedFlush = true;
}

void TWsStreamToProtobuf::SendStartStream(const TString& mime, bool hasSpotter) {
    if (!AudioStreamStarted) {
        NAliceProtocol::TAudio beginStream;
        beginStream.MutableBeginStream()->SetMime(mime);

        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(beginStream.ShortUtf8DebugString());
        AhContext->AddProtobufItem(beginStream, ITEM_TYPE_AUDIO);
        if (hasSpotter) {
            SendStartSpotter();
            StreamStatus = ProcessSpotter;
        } else {
            if (SmartActivationRequest) {
                SmartActivationRequest.Reset();
            }
            StreamStatus = ProcessStream;
        }
        if (SmartActivationRequest) {
            RequestContext.MutableDeviceState()->SetSmartActivation(true);
            Metrics.PushRate("smart_activation_requested");
        }
        AudioStreamStarted = true;
    }
}

void TWsStreamToProtobuf::SendStartSpotter() {
    NAliceProtocol::TAudio spotter;
    spotter.MutableBeginSpotter();

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(spotter.ShortUtf8DebugString());
    AhContext->AddProtobufItem(spotter, ITEM_TYPE_AUDIO);

    if (SmartActivationRequest) {
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostActivationAnnouncementRequest>(SmartActivationRequest->ShortUtf8DebugString());
        AhContext->AddProtobufItem(*SmartActivationRequest, ITEM_TYPE_ACTIVATION_ANNOUNCEMENT_REQUEST);
        AhContext->AddFlag(EDGE_FLAG_SMART_ACTIVATION);
    }
}

void TWsStreamToProtobuf::StartContextLoad(const TMessage& message) {
    ::StartContextLoad(message, SessionContext, RequestContext, AhContext);
}

void TWsStreamToProtobuf::StartCreateOrUpdateBioContextUser(const TMessage& message) {
    {
        // add item for biometry_context loader
        NCachalotProtocol::TYabioContextRequest request;
        auto& recordKey = *request.MutableLoad()->MutableKey();
        recordKey.SetGroupId(GetGroupId(message, RequestContext.GetBiometryOptions().GetGroup()));
        if (SessionContext.GetDeviceInfo().HasDeviceModel()) {
            recordKey.SetDevModel(SessionContext.GetDeviceInfo().GetDeviceModel());
        }
        if (SessionContext.GetDeviceInfo().HasDeviceManufacturer()) {
            recordKey.SetDevManuf(SessionContext.GetDeviceInfo().GetDeviceManufacturer());
        }
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostYabioContextRequest>(request.ShortUtf8DebugString());
        AhContext->AddProtobufItem(request, ITEM_TYPE_YABIO_CONTEXT_REQUEST);
    }
    {
        // build update request command
        NAliceProtocol::TBioContextUpdate updateReq;
        auto& createUser = *updateReq.MutableCreateUser();
        const auto& userId = message.Json["event"]["payload"]["user_id"];
        const auto& requestIds = message.Json["event"]["payload"]["request_ids"];
        if (!userId.IsString()) {
            AddEventException("bad_bio_request", "not found user_id(string) in biometry command for create or update user");
            return;
        }

        createUser.SetUserId(userId.GetString());
        if (!requestIds.IsArray()) {
            AddEventException("bad_bio_request", "not found request_ids(array) in biometry command for create or update user");
            return;
        }

        for (auto& jVal : requestIds.GetArray()) {
            if (jVal.IsString()) {
                createUser.AddRequestIds(jVal.GetString());
            }
        }
        AhContext->AddProtobufItem(updateReq, ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE);
    }
    NeedFlush = true;
}

void TWsStreamToProtobuf::StartLoadBiometryContext(const TMessage& message) {
    // add item for biometry_context loader
    NCachalotProtocol::TYabioContextRequest request;
    auto& recordKey = *request.MutableLoad()->MutableKey();
    recordKey.SetGroupId(GetGroupId(message, RequestContext.GetBiometryOptions().GetGroup()));
    if (SessionContext.GetDeviceInfo().HasDeviceModel()) {
        recordKey.SetDevModel(SessionContext.GetDeviceInfo().GetDeviceModel());
    }
    if (SessionContext.GetDeviceInfo().HasDeviceManufacturer()) {
        recordKey.SetDevManuf(SessionContext.GetDeviceInfo().GetDeviceManufacturer());
    }
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostYabioContextRequest>(request.ShortUtf8DebugString());
    AhContext->AddProtobufItem(request, ITEM_TYPE_YABIO_CONTEXT_REQUEST);
    NeedFlush = true;
}

void TWsStreamToProtobuf::StartRemoveBiometryContextUser(const TMessage& message) {
    // load entire context, remove single user, save context back
    StartLoadBiometryContext(message);

    // build update request command
    NAliceProtocol::TBioContextUpdate updateReq;
    auto& removeUser = *updateReq.MutableRemoveUser();
    auto& userId = message.Json["event"]["payload"]["user_id"];
    if (!userId.IsString()) {
        AddEventException("bad_bio_request", "not found user_id(string) in biometry command for remove user");
        return;
    }

    removeUser.SetUserId(userId.GetString());
    AhContext->AddProtobufItem(std::move(updateReq), ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE);
    NeedFlush = true;
}

bool TWsStreamToProtobuf::BuildRequestContext(const TMessage& message) {
    if (!message.Header) {
        return false;
    }

    // request context can be already partially filled here (PredefinedResults)
    {
        TInstant now = TInstant::Now();
        auto& ts = *RequestContext.MutableCreatingTimestamp();
        ts.set_seconds(now.Seconds());
        ts.set_nanos(now.MicroSecondsOfSecond()*1000);
    }
    {
        // TODO move merge with session context to other function
        // write tests
        // More fields to merge (?)
        RequestContext.MutableAudioOptions()->CopyFrom(SessionContext.GetAudioOptions());
        RequestContext.MutableVoiceOptions()->CopyFrom(SessionContext.GetVoiceOptions());
    }

    auto& header = *RequestContext.MutableHeader();
    auto& additionalOptions = *RequestContext.MutableAdditionalOptions();

    header.SetSessionId(SessionContext.GetSessionId());
    header.SetMessageId(message.Header->MessageId);
    header.SetFullName(ToString(message.Header->Class));

    if (HasManagerSettings) {
        *RequestContext.MutableSettingsFromManager() = SettingsFromManager;
    }

    if (const auto streamId = message.Header->StreamId; streamId != TMessage::INVALID_STREAM_ID) {
        header.SetStreamId(streamId);
    }

    if (const auto refStreamId = message.Header->RefStreamId; refStreamId != TMessage::INVALID_STREAM_ID) {
        header.SetRefStreamId(refStreamId);
    }

    if (const auto* payloadPtr = message.Json.GetValueByPath("event.payload"); payloadPtr && payloadPtr->IsMap()) {
        const auto& payload = *payloadPtr;
        if (const auto* vinsUrl = payload.GetValueByPath("vinsUrl"); vinsUrl && vinsUrl->IsString()) {
            RequestContext.SetVinsUrl(vinsUrl->GetString());
        }
        if (
            const auto* vinsUrl = message.Json.GetValueByPath("uniproxy_options.megamind_url");
            vinsUrl && vinsUrl->IsString()
        ) {
            RequestContext.SetVinsUrl(vinsUrl->GetString());
        }

        if (const auto* formatJson = payload.GetValueByPath("format"); formatJson && formatJson->IsString()) {
            RequestContext.MutableAudioOptions()->SetFormat(formatJson->GetString());
        }
        if (auto realtimeStreamer = payload.GetValueByPath("realtime_streamer"sv)) {
            if (realtimeStreamer->IsMap()) {
                bool streamerEnabled = false;
                if (RequestContext.GetSettingsFromManager().HasApphostedRtsOpus()) {
                    streamerEnabled = RequestContext.GetSettingsFromManager().GetApphostedRtsOpus();
                }
                GetBoolean(*realtimeStreamer, "enabled"sv, &streamerEnabled);
                if (streamerEnabled) {
                    if (auto opus = realtimeStreamer->GetValueByPath("opus"sv)) {
                        if (RequestContext.GetSettingsFromManager().GetApphostedRtsOpus()) {
                            bool enabledRtsOpus;
                            if (GetBoolean(*opus, TStringBuf("enabled"), &enabledRtsOpus) && enabledRtsOpus) {
                                RequestContext.MutableAudioOptions()->SetOpusRealTimeStreamer(true);
                            }
                        }
                        unsigned long long rtsBuffer;  // seconds
                        if (GetUInteger(*opus, TStringBuf("buffer"), &rtsBuffer)) {
                            RequestContext.MutableAudioOptions()->SetRtsBufferSeconds(rtsBuffer);
                        }
                    }
                }
            }
        }
        if (RequestContext.GetSettingsFromManager().HasTtsRts()) {
            RequestContext.MutableAudioOptions()->SetOpusRealTimeStreamer(RequestContext.GetSettingsFromManager().GetTtsRts());
        }
        if (RequestContext.GetSettingsFromManager().HasTtsRtsBufferSec()) {
            RequestContext.MutableAudioOptions()->SetRtsBufferSeconds(RequestContext.GetSettingsFromManager().GetTtsRtsBufferSec());
        }

        if (const auto* val = payload.GetValueByPath("vins_partial"); val && val->IsBoolean()) {
            RequestContext.MutableMegamindOptions()->SetUseAsrPartials(val->GetBoolean());
        }

        {
            // Rewrite to normal parser...
            // Someday...
            static auto getDoubleByPath = [](const auto& json, const TString& path) -> TMaybe<double> {
                if (const auto* valJson = json.GetValueByPath(path)) {
                    if (valJson->IsInteger()) {
                        return valJson->GetInteger();
                    } else if (valJson->IsUInteger()) {
                        return valJson->GetUInteger();
                    } else if (valJson->IsDouble()) {
                        return valJson->GetDouble();
                    }
                }

                return Nothing();
            };

            if (const auto volume = getDoubleByPath(payload, "volume")) {
                RequestContext.MutableVoiceOptions()->SetVolume(*volume);
            }
            if (const auto speed = getDoubleByPath(payload, "speed")) {
                RequestContext.MutableVoiceOptions()->SetSpeed(*speed);
            }

            if (const auto* qualityJson = payload.GetValueByPath("quality"); qualityJson && qualityJson->IsString()) {
                RequestContext.MutableVoiceOptions()->SetQuality(NTtsUtils::VoiceQualityFromString(qualityJson->GetString()));
            }
            if (const auto* emotionJson = payload.GetValueByPath("emotion"); emotionJson && emotionJson->IsString()) {
                RequestContext.MutableVoiceOptions()->SetUnrestrictedEmotion(emotionJson->GetString());
            }
            if (const auto* langJson = payload.GetValueByPath("lang"); langJson && langJson->IsString()) {
                RequestContext.MutableVoiceOptions()->SetLang(langJson->GetString());
            }
            if (const auto* voiceJson = payload.GetValueByPath("voice"); voiceJson && voiceJson->IsString()) {
                RequestContext.MutableVoiceOptions()->SetVoice(voiceJson->GetString());
            }
        }

        if (const auto* val = payload.GetValueByPath("enable_spotter_validation")) {
            RequestContext.MutableAudioOptions()->SetHasSpotter(val->GetBoolean());
        } else {
            const TJsonValue* advancedOptions = nullptr;
            if (!payload.GetValuePointer(TStringBuf("advancedASROptions"), &advancedOptions)) {
                if (!payload.GetValuePointer(TStringBuf("advanced_options"), &advancedOptions)) {
                    payload.GetValuePointer(TStringBuf("advancedOptions"), &advancedOptions);
                }
            }
            if (advancedOptions) {
                if (const auto* val = advancedOptions->GetValueByPath("spotter_validation")) {
                    RequestContext.MutableAudioOptions()->SetHasSpotter(val->GetBoolean());
                }
            }
        }
        if (const auto* val = payload.GetValueByPath("ignore_secondary_context")) {
            RequestContext.MutableAdditionalOptions()->SetIgnoreSecondaryContext(val->GetBoolean());
        }
        if (const auto* val = payload.GetValueByPath("ignore_guest_context")) {
            RequestContext.MutableAdditionalOptions()->SetIgnoreGuestContext(val->GetBoolean());
        }

        if (const auto* dialogId = payload.GetValueByPath("header.dialog_id"); dialogId && dialogId->IsString()) {
            header.SetDialogId(dialogId->GetString());
        }

        if (const auto* reqId = payload.GetValueByPath("header.request_id"); reqId && reqId->IsString()) {
            header.SetReqId(reqId->GetString());
        }

        if (const auto* prevReqId = payload.GetValueByPath("header.prev_req_id"); prevReqId && prevReqId->IsString()) {
            header.SetPrevReqId(prevReqId->GetString());
        }

        if (const auto* smarthomeUid = payload.GetValueByPath("request.additional_options.quasar_auxiliary_config.alice4business.smart_home_uid")) {
            if (smarthomeUid->IsString()) {
                additionalOptions.SetSmarthomeUid(smarthomeUid->GetString());
            } else if (smarthomeUid->IsInteger()) {
                additionalOptions.SetSmarthomeUid(ToString(smarthomeUid->GetInteger()));
            } else if (smarthomeUid->IsUInteger()) {
                additionalOptions.SetSmarthomeUid(ToString(smarthomeUid->GetUInteger()));
            } else {
                LogContext.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "Unknown type of Smarthome Uid: " << smarthomeUid->GetType());
            }
        }

        if (const auto* speakersCount = payload.GetValueByPath("request.additional_options.speakers_count")) {
            if (speakersCount->IsInteger()) {
                additionalOptions.SetSpeakersCount(speakersCount->GetInteger());
            } else if (speakersCount->IsUInteger()) {
                additionalOptions.SetSpeakersCount(speakersCount->GetUInteger());
            } else {
                additionalOptions.SetSpeakersCount(0);
            }
        }

        if (const auto* spotterRms = payload.GetValueByPath("request.additional_options.spotter_rms")) {
            NRmsConverter::ConvertSpotterFeatures(*spotterRms, additionalOptions.MutableSpotterFeatures());
        }
    }

    // Fill experiments flags
    // Client's flags is the 1st priority
    auto& expFlags = *RequestContext.MutableExpFlags();
    auto& baseFlags = *SessionContext.MutableRequestBase()->MutableRequest()->MutableExperiments();
    if (const auto* clientExps = message.Json.GetValueByPath("event.payload.request.experiments")) {
        NAlice::TExperimentsProto proto;
        if (NExpFlags::ParseExperiments(*clientExps, proto)) {
            DLOG("Parsed experiment flags: " << proto);
            baseFlags.MergeFrom(proto);
        } else {
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("Failed to parse request experiments");
        }
    }

    // TODO (paxakor): VOICESERV-4046
    for (const auto& [key, value] : baseFlags.GetStorage()) {
        if (value.HasString()) {
            expFlags.insert({key, value.GetString()});
        } else if (value.HasNumber()) {
            expFlags.insert({key, ToString(value.GetNumber())});
        } else if (value.HasInteger()) {
            expFlags.insert({key, ToString(value.GetInteger())});
        } else if (value.HasBoolean()) {
            expFlags.insert({key, ToString(value.GetBoolean())});
        }
    }

    bool asrContactsEnabled = false;
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "use_contacts_asr")) {
        asrContactsEnabled = true;
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "contacts_after_flags")) {
        AhContext->AddFlag(EDGE_FLAG_CONTACTS_AFTER_FLAGS);
    }

    if (const NJson::TJsonValue* val = message.Json.GetValueByPath("event.payload.request.predefined_contacts")) {
        if (val->IsString()) {
            NAliceProtocol::TContextLoadPredefinedContacts proto;
            proto.SetValue(val->GetString());
            AhContext->AddProtobufItem(proto, ITEM_TYPE_PREDEFINED_CONTACTS);
            AhContext->AddFlag(EDGE_FLAG_PREDEFINED_CONTACTS);
        }
    }

    // WARNING: this code MUST BE placed AFTER filling experiments
    if (SessionContext.HasBiometryOptions()) {
        *RequestContext.MutableBiometryOptions() = SessionContext.GetBiometryOptions();
    }
    if (const auto* val = message.Json.GetValueByPath("event.payload.biometry_classify")) {
        if (val->IsString() && val->GetString().Size()) {
            RequestContext.MutableBiometryOptions()->SetClassify(val->GetString());
        }
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "enable_biometry_classify")) {
        RequestContext.MutableBiometryOptions()->SetClassify("gender,children");
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "disable_biometry_classify")) {
        RequestContext.MutableBiometryOptions()->ClearClassify();
    }

    if (const auto* val = message.Json.GetValueByPath("event.payload.biometry_score")) {
        if (val->IsBoolean()) {
            RequestContext.MutableBiometryOptions()->SetScore(val->GetBoolean());
        } else if (val->IsInteger()) {
            RequestContext.MutableBiometryOptions()->SetScore(val->GetInteger());
        }
    }
    if (const auto* val = message.Json.GetValueByPath("event.payload.vins_scoring")) {
        if (val->IsBoolean()) {
            RequestContext.MutableBiometryOptions()->SetSendScoreToMM(val->GetBoolean());
        } else if (val->IsInteger()) {
            RequestContext.MutableBiometryOptions()->SetSendScoreToMM(val->GetInteger());
        }
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "enable_biometry_scoring")) {
        RequestContext.MutableBiometryOptions()->SetScore(true);
        RequestContext.MutableBiometryOptions()->SetSendScoreToMM(true);
    }
    if (NExpFlags::ExperimentFlagHasTrueValue(RequestContext, "disable_biometry_scoring")) {
        RequestContext.MutableBiometryOptions()->SetScore(false);
        RequestContext.MutableBiometryOptions()->SetSendScoreToMM(false);
    }
    if (RequestContext.GetBiometryOptions().GetScore() && RequestContext.GetBiometryOptions().GetGroup().empty()) {
        // by default use uuid for biometry scoring group_id
        RequestContext.MutableBiometryOptions()->SetGroup(SessionContext.GetUserInfo().GetUuid());
    }

    {
        // WARNING: this code MUST BE placed AFTER filling experiments
        TString smartActivationUserId;
        TString smartActivationDeviceId;
        // smart_activation supported only on voice_input message/graph
        bool smartActivation = message.Header->FullName == TMessage::VINS_VOICE_INPUT;
        if (smartActivation && !NAlice::NCuttlefish::NExpFlags::ConductingExperiment(RequestContext, "supress_multi_activation")) {
            Metrics.PushRate("smart_activation_skipped", "not_enabled");
            smartActivation = false;
        }
        if (const auto* ignoreSmartActivation = message.Json.GetValueByPath("event.payload.dirty_hacks.ignore_smart_activation")) {
            if (ignoreSmartActivation->IsBoolean() && ignoreSmartActivation->GetBoolean()) {
                if (smartActivation) {
                    smartActivation = false;
                    Metrics.PushRate("smart_activation_skipped", "apphosted_asr_mode");
                }
                asrContactsEnabled = false;
            }
        }
        if (smartActivation && !RequestContext.GetAudioOptions().GetHasSpotter()) {
            smartActivation = false;
            Metrics.PushRate("smart_activation_skipped", "no_spotter");
            LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("no spotter");
        }
        if (smartActivation && (additionalOptions.HasSpeakersCount() && additionalOptions.GetSpeakersCount() == 1)) {
            smartActivation = false;
            Metrics.PushRate("smart_activation_skipped", "single_speaker");
            LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("speakers_count=1");
        }
        if (smartActivation && NAlice::NCuttlefish::NExpFlags::ConductingExperiment(RequestContext, "skip_multi_activation_check")) {
            smartActivation = false;
            Metrics.PushRate("smart_activation_skipped", "has_exp_skip_multi_activation_check");
            LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("has experiment skip_multi_activation_check");
        }
        if (const auto* deviceState = message.Json.GetValueByPath("event.payload.request.device_state")) {
            if (smartActivation) {
                if (const auto* smartActivationVal = deviceState->GetValueByPath("smart_activation")) {
                    if (smartActivationVal->IsBoolean()) {
                        smartActivation = smartActivationVal->GetBoolean();
                        if (!smartActivation) {
                            Metrics.PushRate("smart_activation_skipped", "disabled_by_user");
                            LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("disabled by user");
                        }
                    }
                }
            }
            if (const auto* deviceId = deviceState->GetValueByPath("device_id")) {
                if (deviceId->IsString()) {
                    RequestContext.MutableDeviceState()->SetDeviceId(deviceId->GetString());
                }
            }
        }
        if (smartActivation) {
            smartActivationDeviceId = SessionContext.GetDeviceInfo().GetDeviceId();
            if (!smartActivationDeviceId) {
                smartActivationDeviceId = RequestContext.GetDeviceState().GetDeviceId();
            }
            if (!smartActivationDeviceId) {
                LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("no device_id");
                smartActivation = false;
                Metrics.PushRate("smart_activation_skipped", "no_device_id");
            }
        }
        if (smartActivation) {
            smartActivationUserId = SessionContext.GetUserInfo().GetPuid();
            if (!smartActivationUserId) {
                smartActivationUserId = SessionContext.GetUserInfo().GetYuid();
            }
            if (!smartActivationUserId) {
                LogContext.LogEventInfoCombo<NEvClass::SmartActivationSkipped>("no uid");
                smartActivation = false;
                Metrics.PushRate("smart_activation_skipped", "no_uid");
            }
        }
        if (smartActivation) {
            SmartActivationRequest.Reset(new NCachalotProtocol::TActivationAnnouncementRequest);
            FillSmartActivation(
                *SmartActivationRequest,
                RequestContext,
                smartActivationUserId,
                smartActivationDeviceId,
                SessionContext.GetDeviceInfo().GetDeviceModel(),
                &LogContext
            );
        }
    }

    if (asrContactsEnabled) {
        AhContext->AddFlag(EDGE_FLAG_USE_ASR_CONTACTS);
        AhContext->AddItem(NJson::TJsonValue(true), "flag_use_contacts_in_asr");
    }

    // SessionContext is patched with new RequestBase
    // TODO(sparkle): do it better
    AhContext->AddProtobufItem(SessionContext, ITEM_TYPE_SESSION_CONTEXT);
    DLOG("RequestContext: " << RequestContext);
    return true;
}

void TWsStreamToProtobuf::AddRequestContext() {
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostRequestContext>(RequestContext.ShortUtf8DebugString());
    AhContext->AddProtobufItem(RequestContext, ITEM_TYPE_REQUEST_CONTEXT);
    RequestContextSended = true;
    NeedFlush = true;
}

void TWsStreamToProtobuf::AddEventException(const TString& code, const TString& text, const TString& fullText) {
    if (!RequestContextSended) {
        // build&send minimal RequestContext (MessageId need for processing EventException in WS_ADAPTER_OUT)
        auto& header = *RequestContext.MutableHeader();
        if (!header.HasMessageId()) {
            header.SetMessageId(RefMessageId);
        }
        if (!header.HasFullName()) {
            header.SetFullName("system.eventexception");
        }
        AddRequestContext();
    }
    if (code) {
        Metrics.PushRate("error", code);
    }
    if (fullText) {
        // by security reason not use full (exception?) text in response to user, only log it
        LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(
            TStringBuilder() << code << ": " << text << ": " << fullText
        );
    }
    auto directive = CreateEventExceptionEx(WS_ADAPTER_IN, code, text, RefMessageId);
    LogContext.LogEventErrorCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
    AhContext->AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
    NeedFlush = true;
}

TMessage TWsStreamToProtobuf::CreateMessagePatchedWithExperiments(TMessage::TDirection d, const TString& raw) const {
    NJson::TJsonValue json = ReadJson(raw);
    if (SessionContext.GetUserOptions().GetDisableLocalExperiments()) {
        DLOG("Message wasn't patched with local experiments due to user option");
        return TMessage(d, std::move(json));
    }

    const NVoice::NExperiments::TEventPatcher patcher = Experiments.CreatePatcherForSession(
        SessionContext,
        json  // obviously not the first event of a session but we hope it'll be fine
    );
    patcher.Patch(json["event"], SessionContext);
    DLOG("Message was patched with local experiments: " << TJsonAsDense{json});

    if (NJson::TJsonValue* request = json.GetValueByPath("event.payload.request"); request && request->IsMap()) {
        NVoice::NExperiments::TransferUaasTestsFromMegamindCookie(*request);
    }

    return TMessage(d, std::move(json));
}
