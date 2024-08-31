#include "proto_to_ws_stream.h"

#include "asr_result.h"
#include "biometry.h"
#include "megamind.h"
#include "music_match_response.h"
#include "smart_activation.h"
#include "support_functions.h"
#include "tts_generate.h"
#include "tts_generate_response.h"
#include "vins_timings.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/proto_censor/session_context.h>
#include <alice/cuttlefish/library/protos/bio_context_save.pb.h>
#include <alice/library/proto/protobuf.h>

#include <voicetech/library/messages/build.h>
#include <voicetech/library/messages/message_to_wsevent.h>

#include <util/charset/utf8.h>
#include <util/system/byteorder.h>
#include <util/string/cast.h>

using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NAppHostServices::NSupport;
using namespace NJson;
using namespace NVoicetech::NUniproxy2;

namespace {
    void AppendDescription(NAliceProtocol::TWsEvent& wsEvent, const TString& extraDescription) {
        wsEvent.MutableHeader()->SetDescription(wsEvent.GetHeader().GetDescription() + extraDescription);
    }
}


TProtobufToWsStream::TProtobufToWsStream(NAppHost::TServiceContextPtr ctx, TLogContext logContext)
    : AhContext(ctx)
    , Promise(NThreading::NewPromise())
    , LogContext(logContext)
{
    LogContext.LogEventInfoCombo<NEvClass::ProtoToWsStream>(GetGuidAsString(ctx->GetRequestID()));
    DebugInfo_.MutableWsAdapterOut()->SetStart(TInstant::Now().MilliSeconds());
}

void TProtobufToWsStream::OnNextInput() {
    using namespace NAliceProtocol;
    static const TVector<TStringBuf> processingFirst = {
        // WARNING: order of items in this vector is important
        // Do not change it
        // Also read VOICESERV-4206 carefully before change something here

        ITEM_TYPE_SESSION_CONTEXT,
        ITEM_TYPE_REQUEST_CONTEXT,
        ITEM_TYPE_DIRECTIVE,
        ITEM_TYPE_ACTIVATION_LOG,
        ITEM_TYPE_ACTIVATION_LOG_FINAL,
        ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_RESPONSE,
        ITEM_TYPE_MEGAMIND_RESPONSE,  // we want to check if output_speech contains spotter words before processing first tts chunk
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
    TIntrusivePtr<TProtobufToWsStream> self(this);
    AhContext->NextInput().Apply([self](auto hasData) mutable {
        if (!hasData.GetValue()) {
            self->OnEndInput();
            self->Promise.SetValue();
            return;
        }

        self->OnNextInput();
    });
}

bool TProtobufToWsStream::OnAppHostProtoItem(TStringBuf type, const NAppHost::NService::TProtobufItem& item) {
    if (State == Finished) {
        return false;
    }

    Watchmen.OnNewInputItem(type);

    TMaybe<bool> maybeResult = OnAppHostProtoItemImpl(type, item);

    if (Watchmen.WantedCloseGraph() && Watchmen.CanCloseGraph()) {
        ForceAddCloseGraph();
    }

    return maybeResult.Defined() ? *maybeResult : true;
}

TMaybe<bool> TProtobufToWsStream::OnAppHostProtoItemImpl(TStringBuf type, const NAppHost::NService::TProtobufItem& item) {
    using namespace NAliceProtocol;

    if (type == ITEM_TYPE_AUDIO) {
        try {
            NAliceProtocol::TAudio audio;  // tts & other synthes output
            ParseProtobufItem(item, audio);

            if (audio.HasChunk()) {
                NAliceProtocol::TAudio audioExceptChunk = audio;
                audioExceptChunk.ClearChunk();
                LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
            } else {
                LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
            }

            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TAudio(std::move(audio)));
                return true;
            }
            OnAudio(audio);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_audio_exception", TStringBuilder() << "OnAudio error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_TTS_TIMINGS) {
        try {
            NAlice::NTts::NProtobuf::TTimings timings;
            ParseProtobufItem(item, timings);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsTimings>(timings.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAlice::NTts::NProtobuf::TTimings(std::move(timings)));
                return true;
            }
            OnTtsTimings(timings);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            OnNotFatalError("on_tts_timings_exception", TStringBuilder() << "OnTtsTimings error: " << CurrentExceptionMessage());
            return true;
        }
    } else if (type == ITEM_TYPE_ASR_PROTO_RESPONSE) {
        try {
            NAlice::NAsr::NProtobuf::TResponse response;
            ParseProtobufItem(item, response);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostAsrResponse>(response.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAlice::NAsr::NProtobuf::TResponse(std::move(response)));
                return true;
            }
            OnAsrResponse(response);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_asr_response", TStringBuilder() << "OnAsrResponse error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_MUSIC_MATCH_INIT_RESPONSE) {
        try {
            NAlice::NMusicMatch::NProtobuf::TInitResponse initResponse;
            ParseProtobufItem(item, initResponse);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostMusicMatchInitResponse>(initResponse.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAlice::NMusicMatch::NProtobuf::TInitResponse(std::move(initResponse)));
                return true;
            }
            OnMusicMatchInitResponse(initResponse);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_music_match_init_response", TStringBuilder() << "OnMusicMatchInitResponse error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_MUSIC_MATCH_STREAM_RESPONSE) {
        try {
            NAlice::NMusicMatch::NProtobuf::TStreamResponse streamResponse;
            ParseProtobufItem(item, streamResponse);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostMusicMatchStreamResponse>(streamResponse.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAlice::NMusicMatch::NProtobuf::TStreamResponse(std::move(streamResponse)));
                return true;
            }
            OnMusicMatchStreamResponse(streamResponse);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_music_match_stream_response", TStringBuilder() << "OnMusicMatchStreamResponse error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_DIRECTIVE) {
        try {
            NAliceProtocol::TDirective directive;
            ParseProtobufItem(item, directive);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostDirective>(directive.ShortUtf8DebugString());
            if (directive.HasException() && RequestContext_.Defined()) {
                OnDirective(directive);
                return false;
            }

            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TDirective(std::move(directive)));
                return true;
            }
            OnDirective(directive);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_directive", TStringBuilder() << "OnDirective error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_SESSION_CONTEXT) {
        try {
            NAliceProtocol::TSessionContext sessionContext;
            ParseProtobufItem(item, sessionContext);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostSessionContext>(CensoredSessionContextStr(sessionContext));
            OnSessionContext(std::move(sessionContext));
            if (!NeedMoreContext()) {
                ProcessInputQueue();
            }
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_session_context", TStringBuilder() << "OnSessionContext exception: " <<  CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_REQUEST_CONTEXT) {
        try {
            NAliceProtocol::TRequestContext requestContext;
            ParseProtobufItem(item, requestContext);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostRequestContext>(requestContext.ShortUtf8DebugString());
            OnRequestContext(std::move(requestContext));
            if (!NeedMoreContext()) {
                ProcessInputQueue();
            }
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_request_context", TStringBuilder() << "OnRequestContext exception: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_RESPONSE) {
       try {
            NAliceProtocol::TContextSaveResponse contextSaveResponse;
            ParseProtobufItem(item, contextSaveResponse);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostContextSaveResponse>(contextSaveResponse.ShortUtf8DebugString(), /* isImportant = */ true);
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TContextSaveResponse(std::move(contextSaveResponse)));
                return true;
            }
            OnContextSaveImportantResponse(std::move(contextSaveResponse));
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_context_save_important_response", TStringBuilder() << "OnContextSaveImportantResponse exception: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_YABIO_PROTO_RESPONSE) {
        try {
            NAlice::NYabio::NProtobuf::TResponse response;
            ParseProtobufItem(item, response);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioResponse>(response.ShortUtf8DebugString());

            if (response.HasAddDataResponse() && response.GetAddDataResponse().has_biometry_result_for_client()) {
                // Send calculated voiceprint to the client to make a match and define the person who is talking
                OnBiometryResultForClient(response.GetAddDataResponse().biometry_result_for_client());
                return true;
            }

            if (NeedMoreContext()) {
                InputQueue.emplace(new NAlice::NYabio::NProtobuf::TResponse(std::move(response)));
                return true;
            }
            OnYabioResponse(response);
        } catch (...) {
            TString errText = TStringBuilder() << "OnYabioResponse error: " << CurrentExceptionMessage();
            if (IsVoiceInputOrUndefined()) {
                OnNotFatalError("on_yabio_proto_response", errText);
                return true;
            } else {
                // send wsEvent EventException, break stream processing
                AddEventException("on_yabio_proto_response", errText);
                return false;
            }
        }
    } else if (type == ITEM_TYPE_MEGAMIND_RESPONSE) {
        try {
            NAliceProtocol::TMegamindResponse response;
            ParseProtobufItem(item, response);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostMegamindResponse>(response.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TMegamindResponse(std::move(response)));
                return true;
            }
            OnMegamindResponse(response);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_mm_response", TStringBuilder() << "On Megamind response error", CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_YABIO_CONTEXT_SAVED) {
        try {
            NAliceProtocol::TBioContextSaved bioContextSaved;
            ParseProtobufItem(item, bioContextSaved);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioContextSaved>(bioContextSaved.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TBioContextSaved(std::move(bioContextSaved)));
                return true;
            }

            OnBioContextSaved();
        } catch (...) {
            TString errText = TStringBuilder() << "OnBioContextSaved error: " << CurrentExceptionMessage();
            if (IsVoiceInputOrUndefined()) {
                OnNotFatalError("on_yabio_context_saved", errText);
                return true;
            } else {
                // send wsEvent EventException, break stream processing
                AddEventException("on_yabio_context_saved", errText);
                return false;
            }
        }
    } else if (type == ITEM_TYPE_YABIO_CONTEXT) {
        try {
            YabioProtobuf::YabioContext bioContext;
            ParseProtobufItem(item, bioContext);

            TStringStream so;
            so << "group_id=" << bioContext.group_id() << " users=" << bioContext.users().size() << " enrollings=" << bioContext.enrolling().size();
            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioContext>(so.Str());
            if (NeedMoreContext()) {
                InputQueue.emplace(new YabioProtobuf::YabioContext(std::move(bioContext)));
                return true;
            }

            OnBioContext(bioContext);
        } catch (...) {
            TString errText = TStringBuilder() << "OnBioContext error: " << CurrentExceptionMessage();
            if (IsVoiceInputOrUndefined()) {
                OnNotFatalError("on_yabio_context", errText);
                return true;
            } else {
                // send wsEvent EventException, break stream processing
                AddEventException("on_yabio_context", errText);
                return false;
            }
        }
    } else if (type == ITEM_TYPE_STORE_AUDIO_RESPONSE || type == ITEM_TYPE_STORE_AUDIO_RESPONSE_SPOTTER) {
        try {
            NAliceProtocol::TStoreAudioResponse response;
            ParseProtobufItem(item, response);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostStoreAudioResponse>(response.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TStoreAudioResponse(std::move(response)));
                return true;
            }

            OnStoreAudioResponse(response);
        } catch (...) {
            OnNotFatalError("on_store_audio_response", TStringBuilder() << "OnStoreAudioResponse error: " << CurrentExceptionMessage());
            return true;
        }
    } else if (type == ITEM_TYPE_PREDEFINED_ASR_RESULT) {
        try {
            NAliceProtocol::TPredefinedAsrResult result;
            ParseProtobufItem(item, result);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostPredefinedAsrResult>(result.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TPredefinedAsrResult(std::move(result)));
                return true;
            }
            OnPredefinedAsrResult(result);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_predefined_asr_result", TStringBuilder() << "On PredefinedAsrResult error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_ACTIVATION_FINAL_RESPONSE) {
        try {
            NCachalotProtocol::TActivationFinalResponse result;
            ParseProtobufItem(item, result);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostActivationFinalResponse>(result.ShortUtf8DebugString());
            if (NeedMoreContext()) {
                InputQueue.emplace(new NCachalotProtocol::TActivationFinalResponse(std::move(result)));
                return true;
            }

            OnActivationFinalResponse(result);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException("on_activation_final_response", TStringBuilder() << "On SmartActivation response error: " << CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_ACTIVATION_SUCCESSFUL) {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostActivationSuccessful>("-");
        if (NeedMoreContext()) {
            InputQueue.emplace(new NCachalotProtocol::TActivationSuccessful());
            return true;
        }

        OnActivationSuccess();
    } else if (type == ITEM_TYPE_ACTIVATION_LOG) {
        try {
            NCachalotProtocol::TActivationLog log;
            ParseProtobufItem(item, log);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostActivationLog>(log.ShortUtf8DebugString());
            ActivationLog_ = std::move(log);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            OnNotFatalError("on_activation_log", TStringBuilder() << "On ActivationLog response error: " << CurrentExceptionMessage());
            return true;
        }
    } else if (type == ITEM_TYPE_ACTIVATION_LOG_FINAL) {
        try {
            NCachalotProtocol::TActivationLog log;
            ParseProtobufItem(item, log);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostActivationLogFinal>(log.ShortUtf8DebugString());
            ActivationLogFinal_ = std::move(log);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            OnNotFatalError("on_activation_log_final", TStringBuilder() << "On ActivationLog(Final) response error: " << CurrentExceptionMessage());
            return true;
        }
    } else if (type == ITEM_TYPE_UPDATE_CLIENT_ENROLLMENT_DIRECTIVE) {
        try {
            NAliceProtocol::TEnrollmentUpdateDirective enrollmentUpdateDirective;
            ParseProtobufItem(item, enrollmentUpdateDirective);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostEnrollmentUpdateDirective>(enrollmentUpdateDirective.ShortUtf8DebugString());
            OnVoiceprintUpdate(enrollmentUpdateDirective);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            OnNotFatalError("on_voiceprint_update", TStringBuilder() << "On VoiceprintUpdate response error: " << CurrentExceptionMessage());
            return true;
        }
    } else if (type == ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS) {
        try {
            NAliceProtocol::TUniproxyDirectives directives;
            ParseProtobufItem(item, directives);

            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostUniproxyDirective>(directives.ShortUtf8DebugString());

            if (NeedMoreContext()) {
                InputQueue.emplace(new NAliceProtocol::TUniproxyDirectives(std::move(directives)));
                return true;
            }
            OnUniproxy2DirectivesSessionLogs(directives);
        } catch (...) {
            // send wsEvent EventException, break stream processing
            AddEventException(
                "on_uniproxy2_directives_session_logs_exception",
                TStringBuilder() << "OnUniproxy2DirectivesSessionLogs error: " << CurrentExceptionMessage()
            );
            return false;
        }
    } // else ignore unknown item types

    return Nothing();
}

void TProtobufToWsStream::ProcessInputQueue() {
    TString processor;
    try {
        while (InputQueue.size() && State == ProcessInput) {
            if (const NAliceProtocol::TAudio* audioPtr = dynamic_cast<NAliceProtocol::TAudio*>(InputQueue.front().get())) {
                processor = "OnAudio";
                OnAudio(*audioPtr);
            } else if (const NAlice::NTts::NProtobuf::TTimings* timingsPtr = dynamic_cast<NAlice::NTts::NProtobuf::TTimings*>(InputQueue.front().get())) {
                processor = "OnTtsTimings";
                OnTtsTimings(*timingsPtr);
            } else if (const NAlice::NAsr::NProtobuf::TResponse* asrResponsePtr = dynamic_cast<NAlice::NAsr::NProtobuf::TResponse*>(InputQueue.front().get())) {
                processor = "OnAsrResponse";
                OnAsrResponse(*asrResponsePtr);
            } else if (const NAliceProtocol::TDirective* directivePtr = dynamic_cast<NAliceProtocol::TDirective*>(InputQueue.front().get())) {
                processor = "OnDirective";
                OnDirective(*directivePtr);
            } else if (const NAlice::NMusicMatch::NProtobuf::TInitResponse* initResponsePtr = dynamic_cast<NAlice::NMusicMatch::NProtobuf::TInitResponse*>(InputQueue.front().get())) {
                processor = "OnMusicMatchInitResponse";
                OnMusicMatchInitResponse(*initResponsePtr);
            } else if (const NAlice::NMusicMatch::NProtobuf::TStreamResponse* streamResponsePtr = dynamic_cast<NAlice::NMusicMatch::NProtobuf::TStreamResponse*>(InputQueue.front().get())) {
                processor = "OnMusicMatchStreamResponse";
                OnMusicMatchStreamResponse(*streamResponsePtr);
            } else if (const NAliceProtocol::TMegamindResponse* megamindResponsePtr = dynamic_cast<NAliceProtocol::TMegamindResponse*>(InputQueue.front().get())) {
                processor = "OnMegamindResponse";
                OnMegamindResponse(*megamindResponsePtr);
            } else if (const NAliceProtocol::TStoreAudioResponse* storeAudioResponsePtr = dynamic_cast<NAliceProtocol::TStoreAudioResponse*>(InputQueue.front().get())) {
                processor = "OnStoreAudioResponse";
                OnStoreAudioResponse(*storeAudioResponsePtr);
            } else if (const NAliceProtocol::TPredefinedAsrResult* predefinedAsrResultPtr = dynamic_cast<NAliceProtocol::TPredefinedAsrResult*>(InputQueue.front().get())) {
                processor = "OnPredefinedAsrResult";
                OnPredefinedAsrResult(*predefinedAsrResultPtr);
            } else if (const NCachalotProtocol::TActivationFinalResponse* response = dynamic_cast<NCachalotProtocol::TActivationFinalResponse*>(InputQueue.front().get())) {
                processor = "OnActivationFinalResponse";
                OnActivationFinalResponse(*response);
            } else if (dynamic_cast<NCachalotProtocol::TActivationSuccessful*>(InputQueue.front().get())) {
                processor = "OnActivationSuccess";
                OnActivationSuccess();
            } else if (const NAliceProtocol::TContextSaveResponse* response = dynamic_cast<NAliceProtocol::TContextSaveResponse*>(InputQueue.front().get())) {
                processor = "OnContextSaveImportantResponse";
                OnContextSaveImportantResponse(*response);
            } else if (const NAliceProtocol::TUniproxyDirectives* directivesPtr = dynamic_cast<NAliceProtocol::TUniproxyDirectives*>(InputQueue.front().get())) {
                processor = "OnUniproxy2DirectivesSessionLogs";
                OnUniproxy2DirectivesSessionLogs(*directivesPtr);
            }
            InputQueue.pop();
        }
    } catch (...) {
        ythrow yexception() << "ProcessInputQueue at " << processor << ": " << CurrentExceptionMessage();
    }
}

void TProtobufToWsStream::OnEndInput() {
    LogContext.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
    bool isVoiceInput = RequestContext_.Defined() && IsVoiceInputOrUndefined();
    if (isVoiceInput) {
        ForceAddCloseGraph();
    }
    State = Finished;
}

void TProtobufToWsStream::OnBreakProcessing() {
    if (NeedFlush) {
        OnPreFinalMessage();
        AhContext->Flush();
        NeedFlush = false;
    }
    TryAddCloseGraph();
    State = Finished;
}

void TProtobufToWsStream::OnPreFinalMessage() {
    if (OnPreFinalMessageProcessed_) {
        return;
    }

    DebugInfo_.MutableWsAdapterOut()->SetFinish(TInstant::Now().MilliSeconds());
    AddRequestDebugInfo();
    OnPreFinalMessageProcessed_ = true;
}

void TProtobufToWsStream::OnAudio(const NAliceProtocol::TAudio& audio) {
    // Ignore meta info
    // Event exceptions are directly sended to response
    // And there is a separate message for timings

    // Send audio stream
    if (audio.HasBeginStream()) {
        // At first, we need send to user TTS.speak directive with (audio) streamId
        TJsonValue json;
        static const TString tts = "TTS";
        static const TString speak = "Speak";
        auto& payload = BuildDirective(json, tts, speak, RefMessageId_, RefStreamId_);

        {
            // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8519246#L323-327
            payload["format"] = audio.GetBeginStream().GetMime();
            payload["enable_bargin"] = NExpFlags::ExperimentFlagHasTrueValue(*RequestContext_, "enable_bargin");
            payload["lazy_tts_streaming"] = NExpFlags::ExperimentFlagHasTrueValue(*RequestContext_, "enable_lazy_tts_streaming");
            payload["from_cache"] = (audio.GetTtsAggregatorAudioMetaInfo().GetFirstChunkAudioSource() == ::NTts::TAggregatorAudioMetaInfo::TTS_CACHE);
            bool disableSpotter = false;

            if (NExpFlags::ExperimentFlagHasTrueValue(*RequestContext_, "disable_interruption_spotter")) {
                // locig from python: https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8752115#L427-437
                // TODO: check payload.force_interruption_spotter

                disableSpotter = ContainsSpotterWordInOutputSpeech_;

                if (NExpFlags::ExperimentFlagHasTrueValue(*RequestContext_, "force_interruption_spotter_flag")) {
                    disableSpotter = false;
                }
            }

            payload["disableInterruptionSpotter"] = disableSpotter;
        }

        AddMessageToAhContext(TMessage(TMessage::ToClient, std::move(json)));
    } else if (audio.HasChunk()) {
        auto& chunk = audio.GetChunk();
        if (chunk.HasData() && chunk.GetData().size()) {
            if (!DebugInfo_.MutableWsAdapterOut()->HasFirstTtsChunk()) {
                ui64 lag = 0;  // lag to ws_adapter_out start
                ui64 nowMs = TInstant::Now().MilliSeconds();
                if (nowMs > DebugInfo_.MutableWsAdapterOut()->GetStart()) {
                    lag = nowMs - DebugInfo_.MutableWsAdapterOut()->GetStart();
                }
                DebugInfo_.MutableWsAdapterOut()->SetFirstTtsChunk(lag);
                AddLegacyTtsTimings();
            }
            NAliceProtocol::TWsEvent wsEvent;
            {
                TStringStream ss;
                ss.Write(&InetRefStreamId_, sizeof(InetRefStreamId_));
                ss.Write(chunk.GetData().data(), chunk.GetData().size());
                wsEvent.SetBinary(ss.Str());
                if (chunk.HasTimings()) {
                    *wsEvent.MutableAudioChunkTimings() = chunk.GetTimings();
                }
            }
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventBinary>(wsEvent.GetBinary().Size());
            AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
            NeedFlush = true;
        }
    } else if (audio.HasEndStream()) {
        // build streamcontrol for EOS
        OnPreFinalMessage();
        AddMessageToAhContext(BuildStreamControl(TMessage::TStreamControl::ACTION_CLOSE, TMessage::TStreamControl::ReasonOk, RefStreamId_));
        TryAddCloseGraph();
    }
}

void TProtobufToWsStream::OnTtsTimings(const NAlice::NTts::NProtobuf::TTimings& timings) {
    for (const auto& subTimings : timings.GetTimings()) {
        TJsonValue jsonPayload;
        TtsGenerateResponseTimingsToJson(subTimings, timings.GetIsFromCache(), jsonPayload);
        if (jsonPayload.IsDefined()) {  // has timings for user
            TJsonValue jsonMsg;
            auto& payload = BuildDirective(jsonMsg, "TTS", "Timings", RefMessageId_);
            payload.Swap(jsonPayload);
            AddMessageToAhContext(TMessage(TMessage::ToClient, std::move(jsonMsg)));
        }
    }
}

void TProtobufToWsStream::OnAsrResponse(const NAlice::NAsr::NProtobuf::TResponse& response) {
    if (response.HasInitResponse()) {
        auto& initResponse = response.GetInitResponse();
        AsrServerVersion_ = initResponse.GetServerVersion();
        AsrTopic_ = initResponse.GetTopic();
        AsrTopicVersion_ = initResponse.GetTopicVersion();
        AsrHostname_ = initResponse.GetHostname();
        if (!initResponse.GetIsOk()) {
            TStringStream error;
            error << TStringBuf("asr-server error: ");
            if (initResponse.HasErrMsg()) {
                error << initResponse.GetErrMsg();
            } else {
                error << TStringBuf("InitResponse is not ok");
            }
            AddEventException("asr_init_error", error.Str());
            return;
        }
    } else if (response.HasAddDataResponse()) {
        auto& addDataResponse = response.GetAddDataResponse();
        if (!addDataResponse.GetIsOk()) {
            TStringStream error;
            error << TStringBuf("asr-server error: ");
            if (addDataResponse.HasErrMsg()) {
                error << addDataResponse.GetErrMsg();
            } else {
                error << TStringBuf("AddDataResponse is not ok");
            }
            AddEventException("asr_add_data_error", error.Str());
            return;
        }

        auto responseStatus = addDataResponse.GetResponseStatus();
        TString extraDescription;
        if (responseStatus == NAlice::NAsr::NProtobuf::ValidationFailed) {
            AsrSpotterValid_ = false;
            PushMetric("asr_spotter_validation", "failed");
            if (RequestContext_->GetDeviceState().GetSmartActivation()) {
                LogContext.LogEventInfoCombo<NEvClass::DebugMessage>("ignore ValidationFailed from asr (wait smart_activation result)");
                return;  // ignore validation failing result from asr (use smart_activation result instead)
            }

            // send Spotter.Validation (failed) instead ASR.Result
            TJsonValue jsonMsg;
            auto& payload = BuildDirective(jsonMsg, "Spotter", "Validation", RefMessageId_);
            payload["result"] = 0;
            payload["valid"] = 0;
            payload["canceled_cause_of_multiactivation"] = 0;

            static const TString result0 = " result=0";
            extraDescription = result0;

            OnPreFinalMessage();
            TMessage message(TMessage::ToClient, std::move(jsonMsg));
            NAliceProtocol::TWsEvent wsEvent;
            MessageToWsEvent(message, wsEvent);
            AppendDescription(wsEvent, extraDescription);
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
            AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
            NeedFlush = true;
            TryAddCloseGraph();
            return;
        } else if (!ValidationInvokedUsed_ && addDataResponse.GetValidationInvoked()) {
            PushMetric("asr_spotter_validation", "success");
            AsrSpotterValid_ = true;
            ValidationInvokedUsed_ = true;
            if (!RequestContext_->GetDeviceState().GetSmartActivation()) {
                // send separate Spotter.Validation (success) message
                TJsonValue jsonSV;
                auto& payload = BuildDirective(jsonSV, "Spotter", "Validation", RefMessageId_);
                payload["result"] = 1;
                payload["valid"] = 1;
                payload["canceled_cause_of_multiactivation"] = 0;
                TMessage message(TMessage::ToClient, std::move(jsonSV));
                NAliceProtocol::TWsEvent wsEvent;
                static const TString result1 = " result=1";
                MessageToWsEvent(message, wsEvent);
                AppendDescription(wsEvent, result1);

                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
                AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
                NeedFlush = true;
            }
        }

        TJsonValue jsonMsg;
        auto& payload = BuildDirective(jsonMsg, "ASR", "Result", RefMessageId_);
        jsonMsg["directive"]["header"]["eventId"] = RefMessageId_;  // clone strange crutch from python_uniproxy code (see Directive.create_message())
        size_t partialNum = addDataResponse.HasNumber() ? addDataResponse.GetNumber() : AsrPartialNumber_++;
        jsonMsg["directive"]["payload"]["asr_partial_number"] = partialNum;
        TStringOutput soExtra(extraDescription);
        soExtra << " #" << partialNum;
        AsrResponseToJson(response, payload);
        bool hasEouResult = false;
        // for EOU ASR.Result fill metainfo
        if (addDataResponse.GetResponseStatus() == AsrEngineResponseProtobuf::EndOfUtterance) {
            hasEouResult = true;
            AsrEouResultHasEmptyText_ = HasEmptyText(response);
            if (AsrTopic_ && !payload["metainfo"]["topic"].IsDefined()) {
                payload["metainfo"]["topic"] = AsrTopic_;
            }
            if (AsrTopicVersion_ && !payload["metainfo"]["version"].IsDefined()) {
                payload["metainfo"]["version"] = AsrTopicVersion_;
            }
            if (AsrServerVersion_ && !payload["metainfo"]["serverVersion"].IsDefined()) {
                payload["metainfo"]["serverVersion"] = AsrServerVersion_;
            }
            if (AsrHostname_ && !payload["metainfo"]["serverHostname"].IsDefined()) {
                payload["metainfo"]["serverHostname"] = AsrHostname_;
            }
            static const TString eouTrue = " EOU=true";
            soExtra << eouTrue;
        }

        TMessage message(TMessage::ToClient, std::move(jsonMsg));
        NAliceProtocol::TWsEvent wsEvent;
        MessageToWsEvent(message, wsEvent);
        if (extraDescription) {
            AppendDescription(wsEvent, extraDescription);
        }

        if (hasEouResult) {
            if (RequestContext_->MutableDeviceState()->GetSmartActivation()) {
                if (SmartActivationResult_.Defined()) {
                    if (!SmartActivationResult_->GetActivationAllowed()) {
                        // actiation failed, ignore eou result
                        return;
                    }
                } else if (HasSmartActivationError_) {
                    // interpret such errors as success actiovation
                } else {
                    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("postpone asr eou=true (wait smart_activation)");
                    PostPonedAsrEou_.Reset(new NAliceProtocol::TWsEvent(std::move(wsEvent)));
                    PushMetric("smart_activation_postponed_asr_eou"); //TODO: hist? (delay time)
                    return;
                }
            }
            if (AsrEouResultHasEmptyText_) {
                OnPreFinalMessage();
            }
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
            AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
            VoiceInputAsrEouSended_ = true;
            if (VoiceInputBiometryClassifyPostponed_.Defined()) {
                AddVoiceInputBiometryClassification(VoiceInputBiometryClassifyPostponed_.GetRef());
                VoiceInputBiometryClassifyPostponed_.Clear();
            }
            if (AsrEouResultHasEmptyText_) {
                TryAddCloseGraph();
            }
        } else {
            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
            AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        }
        NeedFlush = true;
    } else if (response.HasHeartBeat()) {
        TJsonValue jsonMsg;
        BuildDirective(jsonMsg, "ASR", "HeartBeat", RefMessageId_);
        TMessage message(TMessage::ToClient, std::move(jsonMsg));
        NAliceProtocol::TWsEvent wsEvent;
        MessageToWsEvent(message, wsEvent);

        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        NeedFlush = true;
    }
}

void TProtobufToWsStream::OnDirective(const NAliceProtocol::TDirective& directive) {
    // directive to wsEvent
    if (directive.HasSyncStateResponse()) {
        //TODO:
    } else if (directive.HasMessengerStateResponse()) {
        //TODO:
    } else if (directive.HasException()) {
        OnEventException(directive.GetException());
    } else if (directive.HasGoAway()) {
        //TODO:?
    } else if (directive.HasInvalidAuth()) {
        //TODO:
    } else if (directive.HasLogAckResponse()) {
        AddLogAck();
    } else if (directive.HasRequestDebugInfo()) {
        OnRequestDebugInfo(directive.GetRequestDebugInfo());
    } // else ignore unknown directive
}

void TProtobufToWsStream::OnSessionContext(NAliceProtocol::TSessionContext&& sessionContext) {
    SessionContext_ = std::move(sessionContext);
    Metrics_.Reset(new TSourceMetrics(SessionContext_.GetRef(), "ws_adapter_out"));
}

void TProtobufToWsStream::OnRequestContext(NAliceProtocol::TRequestContext&& requestContext) {
    RequestContext_ = std::move(requestContext);

    if (RequestContext_->GetHeader().HasMessageId()) {
        RefMessageId_ = RequestContext_->GetHeader().GetMessageId();
    }

    RefStreamId_ = RequestContext_->GetHeader().GetRefStreamId();
    InetRefStreamId_ = HostToInet<ui32>(RefStreamId_);
}

void TProtobufToWsStream::OnContextSaveImportantResponse(const NAliceProtocol::TContextSaveResponse& contextSaveResponse) {
    DebugInfo_.MutableWsAdapterOut()->SetContextSaveResponse(TInstant::Now().MilliSeconds() - DebugInfo_.GetWsAdapterOut().GetStart());

    bool hasFailedDirective = false;
    TStringBuilder failedDirectives;
    for (const auto& directive : contextSaveResponse.GetFailedDirectives()) {
        Metrics_->PushRate("failed_directive", directive, "context_save");
        if (!hasFailedDirective) {
            hasFailedDirective = true;
        } else {
            failedDirectives << ", ";
        }
        failedDirectives << directive;
    }

    if (hasFailedDirective) {
        // Metrics and logging in other place
        ythrow yexception() << "Some important context save directives failed: " << ToString(failedDirectives);
    }
}

void TProtobufToWsStream::OnMusicMatchInitResponse(const NAlice::NMusicMatch::NProtobuf::TInitResponse& initResponse) {
    if (!initResponse.GetIsOk()) {
        TStringStream error;
        error << TStringBuf("music match server error: ");
        if (initResponse.HasErrorMessage()) {
            error << initResponse.GetErrorMessage();
        } else {
            error << TStringBuf("InitResponse is not ok");
        }
        AddEventException("music_match_init_response", error.Str());
    }
}

void TProtobufToWsStream::OnMusicMatchStreamResponse(const NAlice::NMusicMatch::NProtobuf::TStreamResponse& streamResponse) {
    auto& musicResult = streamResponse.GetMusicResult();
    if (!musicResult.GetIsOk()) {
        TStringStream error;
        error << TStringBuf("music match server error: ");
        if (musicResult.HasErrorMessage()) {
            error << musicResult.GetErrorMessage();
        } else {
            error << TStringBuf("MusicResult is not ok");
        }
        AddEventException("music_match_stream_response", error.Str());
        return;
    }

    TJsonValue jsonMsg;
    auto& payload = BuildDirective(jsonMsg, "ASR", "MusicResult", RefMessageId_);
    MusicMatchStreamResponseToJson(streamResponse, payload);

    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
}

void TProtobufToWsStream::OnBiometryResultForClient(const YabioOndeviceProtobuf::BiometryResultForClient& biometryResultForClient) {
    TJsonValue jsonMsg;
    auto& payload = BuildDirective(jsonMsg, "System", "MatchVoicePrint", RefMessageId_);
    auto& directives = payload.InsertValue("directives", NJson::TJsonValue(NJson::JSON_ARRAY));
    auto& directive = directives.AppendValue(NJson::TJsonValue());

    directive["name"] = "match_voice_print";
    directive["payload"]["biometry_result"] = NAlice::ProtoToBase64String(biometryResultForClient);

    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
}

void TProtobufToWsStream::OnYabioResponse(const NAlice::NYabio::NProtobuf::TResponse& response) {
    if (IsVoiceInputOrUndefined()) {
        // for vins_voice_input use only classification results (ignore errors & other type responses)
        if (response.HasAddDataResponse()) {
            auto& addDataResponse = response.GetAddDataResponse();
            if (addDataResponse.GetresponseCode() == 200 &&
                (!response.HasForMethod() || response.GetForMethod() == YabioProtobuf::Classify))
            {
                if (VoiceInputAsrEouSended_) {
                    AddVoiceInputBiometryClassification(addDataResponse);
                } else {
                    if (VoiceInputBiometryClassifyPostponed_.Defined()) {
                        AddVoiceInputBiometryClassification(VoiceInputBiometryClassifyPostponed_.GetRef());
                    }
                    VoiceInputBiometryClassifyPostponed_ = addDataResponse;
                }
            }
        }
        return;
    }
    if (response.HasInitResponse()) {
        auto& initResponse = response.GetInitResponse();
        if (initResponse.GetresponseCode() != NAlice::NYabio::NProtobuf::RESPONSE_CODE_OK) {
            TStringStream error;
            error << TStringBuf("yabio-server init request cause error=") << int(initResponse.GetresponseCode());
            if (IsVoiceInputOrUndefined()) {
                OnNotFatalError("yabio_init_response", error.Str());
            } else {
                AddEventException("yabio_init_response", error.Str());
            }
        }
    } else if (response.HasAddDataResponse()) {
        auto& addDataResponse = response.GetAddDataResponse();
        if (addDataResponse.GetresponseCode() != NAlice::NYabio::NProtobuf::RESPONSE_CODE_OK) {
            TStringStream error;
            error << TStringBuf("yabio-server (add data) error=") << int(addDataResponse.GetresponseCode());
            if (IsVoiceInputOrUndefined()) {
                OnNotFatalError("yabio_init_response", error.Str());
            } else {
                AddEventException("yabio_add_data_reponse", error.Str());
            }
        } else {
            TJsonValue jsonMsg;
            TString responseType = "Classification";
            NAlice::NYabio::NProtobuf::EMethod method = NAlice::NYabio::NProtobuf::METHOD_CLASSIFY;
            if (response.HasForMethod() && response.GetForMethod() == NAlice::NYabio::NProtobuf::METHOD_SCORE) {
                method = NAlice::NYabio::NProtobuf::METHOD_SCORE;
                responseType = "IdentifyComplete";
            }
            auto& payload = BuildDirective(jsonMsg, "Biometry", responseType, RefMessageId_);
            // jsonMsg["directive"]["header"]["eventId"] = RefMessageId;  // clone strange crutch from python_uniproxy code (see Directive.create_message())
            BiometryAddDataResponseToJson(addDataResponse, payload, method);
            TMessage message(TMessage::ToClient, std::move(jsonMsg));
            NAliceProtocol::TWsEvent wsEvent;
            MessageToWsEvent(message, wsEvent);

            LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
            AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        }
        NeedFlush = true;
    }
}

void TProtobufToWsStream::AddVoiceInputBiometryClassification(const NAlice::NYabio::NProtobuf::TAddDataResponse& addDataResponse) {
    TJsonValue jsonMsg;
    auto& payload = BuildDirective(jsonMsg, "Biometry", "Classification", RefMessageId_);
    BiometryAddDataResponseToJson(addDataResponse, payload, NAlice::NYabio::NProtobuf::METHOD_CLASSIFY);
    payload["partial_number"] = VoiceInputYabioPartialNumber_++;

    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
}

void TProtobufToWsStream::OnBioContext(const YabioProtobuf::YabioContext& bioContext) {
    OnPreFinalMessage();
    TJsonValue jsonMsg;
    auto& payload = BuildDirective(jsonMsg, "Biometry", "Users", RefMessageId_);
    payload["status"] = "ok";
    auto& users = payload.InsertValue("users", NJson::JSON_ARRAY);
    for (auto& user : bioContext.users()) {
        TJsonValue jsonRec;
        jsonRec["user_id"] = user.user_id();
        users.AppendValue(std::move(jsonRec));
    }
    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
    TryAddCloseGraph();
}

void TProtobufToWsStream::OnBioContextSaved() {
    TString biometryResponse;
    if (RequestContext_.GetRef().GetHeader().GetFullName() == TMessage::BIOMETRY_CREATE_OR_UPDATE_USER) {
        biometryResponse = "UserCreation";
    } else if (RequestContext_.GetRef().GetHeader().GetFullName() == TMessage::BIOMETRY_REMOVE_USER) {
        biometryResponse = "UserRemoved";
    }
    if (biometryResponse) {
        OnPreFinalMessage();
        TJsonValue jsonMsg;
        auto& payload = BuildDirective(jsonMsg, "Biometry", biometryResponse, RefMessageId_);
        payload["status"] = "ok";
        TMessage message(TMessage::ToClient, std::move(jsonMsg));
        NAliceProtocol::TWsEvent wsEvent;
        MessageToWsEvent(message, wsEvent);
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        NeedFlush = true;
        TryAddCloseGraph();
    }
}

bool TProtobufToWsStream::OutputSpeechContainsSpotterWord(const TString& outputSpeech) {
    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8752115#L42
    const TString& lower = ToLowerUTF8(outputSpeech);

    if (lower.find("яндекс новост") != TString::npos) {
        return false;
    }

    if (lower.find("алиса") != TString::npos) {
        return true;
    }

    if (lower.find("яндекс") != TString::npos) {
        return true;
    }
    return false;
}

void TProtobufToWsStream::OnMegamindResponse(const NAliceProtocol::TMegamindResponse& response) {
    OnActivationSuccess();

    HasMegamindResponse_ = true;
    Y_ENSURE(SessionContext_);
    Y_ENSURE(RequestContext_);

    const TMessage msg = BuildVinsResponse(RefMessageId_, response.GetRawJsonResponse());
    NAliceProtocol::TWsEvent wsEvent = MessageToWsEvent(msg);

    const TString& outputSpeech = response.GetProtoResponse().GetVoiceResponse().GetOutputSpeech().GetText();
    const bool needTts = outputSpeech.size();
    ContainsSpotterWordInOutputSpeech_ = OutputSpeechContainsSpotterWord(outputSpeech);
    DebugInfo_.MutableWsAdapterOut()->SetMegamindResponse(TInstant::Now().MilliSeconds() - DebugInfo_.GetWsAdapterOut().GetStart());
    if (!needTts) {
        OnPreFinalMessage();
        AddLegacyVinsTimings();
        AddLegacyTtsTimings();
    } else {
        AddLegacyVinsTimings();
    }
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
    if (!needTts) {
        TryAddCloseGraph();
    }
}

void TProtobufToWsStream::OnStoreAudioResponse(const NAliceProtocol::TStoreAudioResponse& response) {
    Y_ENSURE(RequestContext_);

    if (!response.HasKey()) {
        PushMetric("store_audio_response", "no_key");
        return;
    }

    // emit Uniproxy2 SessionLog/Stream directive (for log)
    NAliceProtocol::TUniproxyDirective directive;
    auto& directiveSessionLog = *directive.MutableSessionLog();
    directiveSessionLog.SetName("Stream");
    directiveSessionLog.SetAction("stream");
    TJsonValue streamValue;
    {
        TString url = TStringBuilder() << "http://storage-int.mds.yandex.net:80/get-speechbase/" << response.GetKey();
        streamValue["MDS"] = url;
    }
    streamValue["format"] = RequestContext_->GetAudioOptions().GetFormat();
    streamValue["isSpotter"] = response.GetIsSpotter();
    auto& requestHeader = RequestContext_->GetHeader();
    streamValue["messageId"] = requestHeader.GetMessageId();
    if (requestHeader.HasStreamId()) {
        streamValue["streamId"] = requestHeader.GetStreamId();
    }
    // not fill "vins_message_id":null (mean deprecate?!)
    directiveSessionLog.SetValue(WriteJson(streamValue, /* formatOutput */ false));
    SendSessionLog(directive);
    NeedFlush = true;
}

void TProtobufToWsStream::OnPredefinedAsrResult(const NAliceProtocol::TPredefinedAsrResult& result) {
    TJsonValue jsonMsg;
    auto& payload = BuildDirective(jsonMsg, "ASR", "Result", RefMessageId_);
    ReadJsonTree(result.GetPayload(), &payload);

    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);
    AppendDescription(wsEvent, "(predefined)");

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
}

void TProtobufToWsStream::OnRequestDebugInfo(const NAliceProtocol::TRequestDebugInfo& debugInfo) {
    DebugInfo_.MergeFrom(debugInfo);
}

void TProtobufToWsStream::OnActivationFinalResponse(const NCachalotProtocol::TActivationFinalResponse& smartActivationResponse) {
    if (!RequestContext_->GetDeviceState().GetSmartActivation()) {
        return;
    }

    SmartActivationResult_ = smartActivationResponse;
    bool activationSuccess;
    TryUseSmartActivationLog();
    auto wsEvent = BuildSpotterValidation(activationSuccess, smartActivationResponse, ActivationLogFinal_, AsrSpotterValid_, RefMessageId_);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
    SpotterValidationSended_ = true;
    if (activationSuccess) {
        if (AsrSpotterValid_.Defined()) {
            if (AsrSpotterValid_.GetRef()) {
                PushMetric("smart_activation_success", "confirm_valid_asr_spotter");
            } else {
                PushMetric("smart_activation_success", "override_invalid_asr_spotter");
            }
        } else {
            PushMetric("smart_activation_success", "todo");
        }
        TryUsePostPonedAsrEou();
    } else {
        if (AsrSpotterValid_.Defined()) {
            if (AsrSpotterValid_.GetRef()) {
                PushMetric("smart_activation_rejected", "override_valid_asr_spotter");
            } else {
                PushMetric("smart_activation_rejected", "confirm_invalid_asr_spotter");
            }
        } else {
            PushMetric("smart_activation_rejected", "todo");
        }
    }
}

void TProtobufToWsStream::OnActivationSuccess() {  // fallback method for smart_activation (in not has right response from smart_activation)
    if (!RequestContext_->GetDeviceState().GetSmartActivation()) {
        return;
    }

    if (SpotterValidationSended_) {
        return;
    }

    if (SmartActivationResult_) {
        return;
    }

    HasSmartActivationError_ = true;
    TryUseSmartActivationLog();
    if (AsrSpotterValid_.Defined()) {
        if (AsrSpotterValid_.GetRef()) {
            PushMetric("smart_activation_success", "fallback_confirm_valid_asr_spotter");
        } else {
            PushMetric("smart_activation_success", "fallback_override_invalid_asr_spotter");
        }
    } else {
        PushMetric("smart_activation_success", "todo");
    }
    // smart activation nodes failed on exection, so we receive stub
    auto wsEvent = BuildFallbackSpotterValidation(ActivationLogFinal_.Defined() ? ActivationLogFinal_ : ActivationLog_, AsrSpotterValid_, RefMessageId_);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
    SpotterValidationSended_ = true;
    TryUsePostPonedAsrEou();
}

void TProtobufToWsStream::OnVoiceprintUpdate(const NAliceProtocol::TEnrollmentUpdateDirective& directive) {
    TJsonValue jsonMsg;
    auto& directiveJson = BuildDirective(jsonMsg, "System", "UpdateVoicePrints", RefMessageId_)
        .InsertValue("directives", NJson::TJsonValue(NJson::JSON_ARRAY))
        .AppendValue(NJson::TJsonMap());

    directiveJson["name"] = "update_voice_prints";
    directiveJson["payload"]
        .InsertValue("voiceprints", NJson::TJsonValue(NJson::JSON_ARRAY))
        .AppendValue(JsonFromProto(directive));

    TMessage message(TMessage::ToClient, std::move(jsonMsg));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
}

void TProtobufToWsStream::OnUniproxy2DirectivesSessionLogs(const NAliceProtocol::TUniproxyDirectives& directives) {
    for (const auto& directive : directives.GetDirectives()) {
        SendSessionLog(directive);
    }
    if (!directives.GetDirectives().empty()) {
        NeedFlush = true;
    }
}

void TProtobufToWsStream::SendSessionLog(const NAliceProtocol::TUniproxyDirective& directive) {
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostUniproxyDirective>(directive.ShortUtf8DebugString());
    AhContext->AddProtobufItem(directive, ITEM_TYPE_UNIPROXY2_DIRECTIVE);
    PushMetric("session_log", "sent");
}

void TProtobufToWsStream::TryUsePostPonedAsrEou() {
    if (!PostPonedAsrEou_) {
        return;
    }

    if (AsrEouResultHasEmptyText_) {
        OnPreFinalMessage();
    }
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(PostPonedAsrEou_->ShortUtf8DebugString());
    AhContext->AddProtobufItem(*PostPonedAsrEou_, ITEM_TYPE_WS_MESSAGE);
    VoiceInputAsrEouSended_ = true;
    NeedFlush = true;
    PostPonedAsrEou_.Reset();

    if (VoiceInputBiometryClassifyPostponed_.Defined()) {
        AddVoiceInputBiometryClassification(VoiceInputBiometryClassifyPostponed_.GetRef());
        VoiceInputBiometryClassifyPostponed_.Clear();
    }
    if (AsrEouResultHasEmptyText_) {
        TryAddCloseGraph();
    }
}

void TProtobufToWsStream::TryUseSmartActivationLog() {
    // special logging spotter activation
    NAliceProtocol::TUniproxyDirective directive;
    try {
        if (ActivationLogFinal_.Defined()) {
            directive = BuildSessionLogMultiActivation(ActivationLogFinal_.GetRef(), RefMessageId_);
        } else if (ActivationLog_.Defined()) {
            directive = BuildSessionLogMultiActivation(ActivationLog_.GetRef(), RefMessageId_);
        } else {
            return;
        }
    } catch (...) {
        //TODO: metrics
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "fail BuildSessionLogMultiActivation: " << CurrentExceptionMessage());
        return;
    }

    SendSessionLog(directive);
    NeedFlush = true;
}

namespace {
    TString ToStringJsonWithFloatPointDigits(const TJsonValue& j, ui32 precisionAfterPoint = 3) {
        TStringStream ss;  // we need specially configured json writer for guaranty tail '.0' for ALICEINFRA-802
        TJsonWriterConfig jCfg;
        jCfg.ValidateUtf8 = false;
        jCfg.DoubleNDigits = precisionAfterPoint;
        jCfg.FloatNDigits = precisionAfterPoint;
        jCfg.FloatToStringMode = PREC_POINT_DIGITS;
        WriteJson(&ss, &j, jCfg);
        return ss.Str();
    }
}

void TProtobufToWsStream::AddLegacyVinsTimings() {
    // emit Uniproxy2 SessionLog/Directive directive (for log)
    NAliceProtocol::TUniproxyDirective directive;
    auto& directiveSessionLog = *directive.MutableSessionLog();
    directiveSessionLog.SetName("Directive");
    directiveSessionLog.SetAction("response");
    TJsonValue streamValue;
    TJsonValue& header = streamValue["header"];
    header["namespace"] = "System";
    header["name"] = "UniproxyVinsTimings";
    header["refMessageId"] = RefMessageId_;
    TJsonValue& payload = streamValue["payload"];
    BuildLegacyVinsTimings(payload, DebugInfo_);
    directiveSessionLog.SetValue(ToStringJsonWithFloatPointDigits(streamValue)); // ALICEINFRA-802
    SendSessionLog(directive);
    if (NAlice::NCuttlefish::NExpFlags::ConductingExperiment(RequestContext_.GetRef(), "uniproxy_vins_timings")) {
        // send to client vins timings message
        TJsonValue jsonMsg;
        auto& payload2 = BuildDirective(jsonMsg, "Vins", "UniproxyVinsTimings", RefMessageId_);
        payload2.Swap(payload);
        payload2["params"]["message_id"] = RefMessageId_;  // support python_uniproxy format :(
        if (SessionContext_.Defined()) {
            payload2["params"]["uuid"] = SessionContext_->GetUserInfo().GetUuid();  // support python_uniproxy format :(
        }
        TMessage message(TMessage::ToClient, std::move(jsonMsg));
        NAliceProtocol::TWsEvent wsEvent;
        MessageToWsEvent(message, wsEvent);
        wsEvent.SetText(ToStringJsonWithFloatPointDigits(message.Json)); // ALICEINFRA-802
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    }
    NeedFlush = true;
}

void TProtobufToWsStream::AddLegacyTtsTimings() {
    // emit Uniproxy2 SessionLog/Directive directive (for log)
    NAliceProtocol::TUniproxyDirective directive;
    auto& directiveSessionLog = *directive.MutableSessionLog();
    directiveSessionLog.SetName("Directive");
    directiveSessionLog.SetAction("response");
    TJsonValue streamValue;
    TJsonValue& header = streamValue["header"];
    header["namespace"] = "System";
    header["name"] = "UniproxyTTSTimings";
    header["refMessageId"] = RefMessageId_;
    TJsonValue& payload = streamValue["payload"];
    BuildLegacyTtsTimings(payload, DebugInfo_);
    directiveSessionLog.SetValue(ToStringJsonWithFloatPointDigits(streamValue)); // ALICEINFRA-802
    SendSessionLog(directive);
    if (NAlice::NCuttlefish::NExpFlags::ConductingExperiment(RequestContext_.GetRef(), "uniproxy_vins_timings")) {
        // send to client vins timings message
        TJsonValue jsonMsg;
        auto& payload2 = BuildDirective(jsonMsg, "TTS", "UniproxyTTSTimings", RefMessageId_);
        payload2.Swap(payload);
        payload2["params"]["message_id"] = RefMessageId_;  // support python_uniproxy format :(
        if (SessionContext_.Defined()) {
            payload2["params"]["uuid"] = SessionContext_->GetUserInfo().GetUuid();  // support python_uniproxy format :(
        }
        TMessage message(TMessage::ToClient, std::move(jsonMsg));
        NAliceProtocol::TWsEvent wsEvent;
        MessageToWsEvent(message, wsEvent);
        wsEvent.SetText(ToStringJsonWithFloatPointDigits(message.Json)); // ALICEINFRA-802
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    }
    NeedFlush = true;
}

bool TProtobufToWsStream::IsVoiceInputOrUndefined() {
    return !RequestContext_.Defined() || RequestContext_.GetRef().GetHeader().GetFullName() == TMessage::VINS_VOICE_INPUT;
}

void TProtobufToWsStream::ForceAddCloseGraph() {
    if (CloseGraphSended_) {
        return;
    }

    NAliceProtocol::TUniproxyDirective directive;
    directive.MutableCloseGraph();
    LogContext.LogEventInfoCombo<NEvClass::CloseGraphDirective>();
    AhContext->AddProtobufItem(directive, ITEM_TYPE_UNIPROXY2_DIRECTIVE);
    CloseGraphSended_ = true;
    NeedFlush = true;
}

void TProtobufToWsStream::TryAddCloseGraph() {
    bool isVoiceInput = RequestContext_.Defined() && IsVoiceInputOrUndefined();
    if (isVoiceInput && !Watchmen.CanCloseGraph()) {
        return;
    }

    ForceAddCloseGraph();
}

void TProtobufToWsStream::AddEventException(const TString& code, const TString& text, const TString& fullText) {
    if (State == Finished) {
        return; // too late
    }

    if (code && Metrics_) {
        Metrics_->PushRate("error", code);
    }
    if (fullText) {
        // by security reason not use full (exception?) text in response to user, only log it
        LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(
            TStringBuilder() << code << ": " << text << ": " << fullText
        );
    }
    OnEventException(CreateEventExceptionEx("WS_ADAPTER_OUT", code, text, RefMessageId_).GetException());
}

void TProtobufToWsStream::OnNotFatalError(const TString& code, const TString& text) {
    if (code && Metrics_) {
        Metrics_->PushRate("not_fatal_error", code);
    }
    LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << code << ": " << text);
}

void TProtobufToWsStream::OnEventException(const NAliceProtocol::TEventException& exc) {
    TStringStream err;
    err << "directive exception:";
    if (exc.HasScope()) {
        err << " scope=" << exc.GetScope();
    }
    if (exc.HasCode()) {
        err << " code=" << exc.GetCode();
        DebugInfo_.MutableWsAdapterOut()->SetErrorCode(exc.GetCode());
    }
    if (exc.HasText()) {
        err << " text=" << exc.GetText();
        DebugInfo_.MutableWsAdapterOut()->SetErrorText(exc.GetText());
    }
    OnPreFinalMessage();  // flush DebugInfo
    State = Finished;

    try {
        auto wsEvent = MessageToWsEvent(BuildEventException(err.Str(), RefMessageId_));
        LogContext.LogEventErrorCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        NeedFlush = true;
    } catch (...) {
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "TProtobufToWsMessage error: " << CurrentExceptionMessage());
    }
    TryAddCloseGraph();
}

void TProtobufToWsStream::AddLogAck() {
    try {
        auto wsEvent = MessageToWsEvent(BuildLogAck(RefMessageId_));

        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
        AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
        NeedFlush = true;
    } catch (...) {
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "TProtobufToWsMessage error: " << CurrentExceptionMessage());
    }
}

void TProtobufToWsStream::AddMessageToAhContext(const TMessage& msg) {
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(msg, wsEvent);

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostWsEventText>(wsEvent.ShortUtf8DebugString());
    AhContext->AddProtobufItem(wsEvent, ITEM_TYPE_WS_MESSAGE);
    NeedFlush = true;
}

void TProtobufToWsStream::AddRequestDebugInfo() {
    try {
        NJson::TJsonValue rec;
        rec["type"] = "RequestDebugInfo";
        rec["backend"] = "ws_adapter_out";
        rec["payload"] = NAlice::JsonFromProto(DebugInfo_);
        NAliceProtocol::TUniproxyDirective directive;
        auto& sessionLogRecord = *directive.MutableSessionLog();
        sessionLogRecord.SetName("Directive");
        sessionLogRecord.SetValue(NJson::WriteJson(rec, false, true));
        sessionLogRecord.SetAction("response");
        SendSessionLog(directive);
        NeedFlush = true;
    } catch (...) {
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "fail build/send RequestDebugInfo: " << CurrentExceptionMessage());
    }
}
