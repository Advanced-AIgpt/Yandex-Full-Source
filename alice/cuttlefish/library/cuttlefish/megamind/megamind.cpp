#include "megamind.h"
#include "utils.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_save/client/starter.h>
#include <alice/cuttlefish/library/cuttlefish/stream_converter/tts_generate.h>
#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/proto_censor/context.h>
#include <alice/cuttlefish/library/proto_censor/megamind.h>
#include <alice/cuttlefish/library/proto_censor/session_context.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/context_save.pb.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <google/protobuf/struct.pb.h>

#include <voicetech/library/itags/itags.h>
#include <voicetech/library/messages/censor.h>
#include <voicetech/library/proto_api/yabio.pb.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/json/json_writer.h>

#include <google/protobuf/wrappers.pb.h>

#include <util/generic/string_hash.h>
#include <util/random/random.h>
#include <util/string/cast.h>

#undef DLOG
#define DLOG(args)

using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

    constexpr TStringBuf UNIPROXY_ACTION_DIRECTIVE_TYPE = "uniproxy_action";
    constexpr TStringBuf DEFER_APPLY_DIRECTIVE_NAME = "defer_apply";
    constexpr TStringBuf UPDATE_MEMENTO_DIRECTIVE_NAME = "update_memento";


    // Hack for VOICESERV-4176
    // TODO(VOICESERV-4205): Do it better
    bool IsDirectiveImportant(const NAlice::NSpeechKit::TDirective& directive) {
        if (directive.GetName() == NAlice::NCuttlefish::NContextSaveClient::SAVE_USER_AUDIO_DIRECTIVE_NAME) {
            return true;
        }

        if (directive.GetName() == UPDATE_MEMENTO_DIRECTIVE_NAME) {
            const auto& payload = directive.GetPayload();
            if (payload.fields().contains("user_objects")) {
                const google::protobuf::Value& userObject = payload.fields().at("user_objects");

                if (userObject.has_string_value()) {
                    try {
                        TString realValue = Base64Decode(userObject.string_value());
                        if (realValue.find("alice-time-capsule") != TString::npos) {
                            return true;
                        }
                    } catch (...) {
                        // ¯\_(ツ)_/¯
                    }
                }
            }
        }

        return false;
    }

    // TODO(sparkle): do better
    NAliceProtocol::TMegamindResponse PatchMegamindResponse(
        const NAliceProtocol::TRequestContext& requestContext,
        const NAliceProtocol::TMegamindResponse& proto,
        const TSubrequest& req
    ) {
        if (proto.HasProtoResponse() && proto.GetProtoResponse().SessionsSize() > 0) {
            NAliceProtocol::TMegamindResponse patched = proto;
            patched.MutableProtoResponse()->ClearSessions();

            NJson::TJsonValue json = NAlice::JsonFromString(patched.GetRawJsonResponse());

            // VOICESERV-4189
            if (!NAlice::NCuttlefish::NExpFlags::ConductingExperiment(requestContext, "stateless_uniproxy_session")) {
                if (json.Has("sessions")) {
                    json.EraseValue("sessions");
                }
            }

            // VOICESERV-4182
            {
                if (req.PartialNumbers.AsrPartialNumber.Defined()) {
                    int asrPartialNumber = req.PartialNumbers.AsrPartialNumber.GetRef();
                    json.SetValueByPath("header.asr_partial_number", NJson::TJsonValue(asrPartialNumber));
                }

                if (req.PartialNumbers.BioScoringPartialNumber.Defined()) {
                    int bioScoringPartialNumber = req.PartialNumbers.BioScoringPartialNumber.GetRef();
                    json.SetValueByPath("header.scoring_partial_number", NJson::TJsonValue(bioScoringPartialNumber));
                }

                if (req.PartialNumbers.BioClassificationPartialNumber.Defined()) {
                    int bioClassificationPartialNumber = req.PartialNumbers.BioClassificationPartialNumber.GetRef();
                    json.SetValueByPath(
                        "header.classification_partial_number",
                        NJson::TJsonValue(bioClassificationPartialNumber)
                    );
                }
            }

            patched.SetRawJsonResponse(NAlice::JsonToString(json, /* validateUtf8 = */ false));

            return patched;
        }

        return proto;
    }

    struct TInvalidMegamindDirective: public yexception {
    };

    const TString RequestPhaseNode(ERequestPhase phase) {
        if (phase == ERequestPhase::RUN) {
            static const TString run{"MEGAMIND_RUN"};
            return run;
        } else if (phase == ERequestPhase::APPLY) {
            static const TString apply{"MEGAMIND_APPLY"};
            return apply;
        } else {
            static const TString s{"MEGAMIND_?"};
            return s;
        }
    }

    TString GetIntentName(const NAliceProtocol::TMegamindResponse& response) {
        for (auto& meta : response.GetProtoResponse().GetResponse().GetMeta()) {
            if (meta.GetType() == TStringBuf("analytics_info") && meta.HasIntent()) {
                return meta.GetIntent();
            }
        }
        for (auto& [k, info] : response.GetProtoResponse().GetMegamindAnalyticsInfo().GetAnalyticsInfo()) {
            if (const TString intent = info.GetScenarioAnalyticsInfo().GetIntent()) {
                return intent;
            }
        }
        return {};  //TODO: ? try get intent from another place (not found response.GetFeatures().GetFormInfo().GetIntent())
    }
}

TMegamindServant::TMegamindServant(
    const NAliceCuttlefishConfig::TConfig& config,
    NAppHost::TServiceContextPtr serviceCtx,
    TLogContext logContext,
    const TString& sourceName
)
    : Config(config)
    , RequestHandler(std::move(serviceCtx), NThreading::NewPromise())
    , LogContext(logContext)
    , Metrics(AhContext(), sourceName)
    , SpeakerService(Metrics, logContext)
{
}

void TMegamindServant::OnNextInputSafe() {
    if (!RequestHandler.TryBeginProcessing()) {
        return;
    }

    try {
        OnNextInput();
    } catch (...) {
        OnInternalError(TStringBuilder() << "unexpected exception from TMegamindServant::OnNextInput: " << CurrentExceptionMessage(), "exception_next_input");
    }
    RequestHandler.EndProcessing();
}

void TMegamindServant::OnNextInput() {
    // updating local items
    if (!AppHostParams) {
        if (const auto* appHostParams = AhContext().FindFirstItem(NAppHost::APP_HOST_PARAMS_TYPE)) {
            AppHostParams = *appHostParams;
            TString logText{NJson::WriteJson(*appHostParams, /* formatOutput */ false, /* sortkeys */ true)};
            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostParams>(logText);
            DLOG(logText);
        }
    }
    if (!SessionCtx && AhContext().HasProtobufItem(ITEM_TYPE_SESSION_CONTEXT)) {
        SessionCtx = AhContext().GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT);
        NAliceProtocol::TSessionContext censoredContext(*SessionCtx);
        Censore(censoredContext);
        TString logText{censoredContext.ShortUtf8DebugString()};
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostSessionContext>(logText);
        DLOG(logText);
    }
    if (!RequestCtx && AhContext().HasProtobufItem(ITEM_TYPE_REQUEST_CONTEXT)) {
        RequestCtx = AhContext().GetOnlyProtobufItem<NAliceProtocol::TRequestContext>(ITEM_TYPE_REQUEST_CONTEXT);
        TString logText{RequestCtx->ShortUtf8DebugString()};
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostRequestContext>(logText);
        DLOG(logText);
        TrySetupExperiments();  // if has Request update it with experiments list
        OnReceiveRequestContext(*RequestCtx);
    }
    if (AhContext().HasProtobufItem(ITEM_TYPE_ALICE_LOGGER_OPTIONS)) {
        AliceLoggerOptions = AhContext().GetOnlyProtobufItem<NAlice::TLoggerOptions>(ITEM_TYPE_ALICE_LOGGER_OPTIONS);
    }
    if (!ContextLoadResponse && AhContext().HasProtobufItem(ITEM_TYPE_CONTEXT_LOAD_RESPONSE)) {
        ContextLoadResponse = AhContext().GetOnlyProtobufItem<NAliceProtocol::TContextLoadResponse>(ITEM_TYPE_CONTEXT_LOAD_RESPONSE);
        NAliceProtocol::TContextLoadResponse censoredResponse(*ContextLoadResponse);
        Censore(censoredResponse);
        TString logText{censoredResponse.ShortUtf8DebugString()};
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostContextLoadResponse>(logText);
        DLOG(logText);
        if (ContextLoadResponse->HasMegamindSessionResponse()) {
            const auto& resp = ContextLoadResponse->GetMegamindSessionResponse();
            if (resp.HasMegamindSessionLoadResp()) {
                LogContext.LogEvent(
                    NEvClass::InfoMessage(
                        TStringBuilder()
                            << "We loaded megamind session (md5 hash "
                            << MD5::Calc(resp.GetMegamindSessionLoadResp().GetData()) << ")"
                    )
                );
            }
        }
    }

    SpeakerService.OnNextInput(AhContext());

    // try to build RequestBuilder and MegamindClient
    if (!MegamindClient) {
        if (SessionCtx && RequestCtx && ContextLoadResponse) {
            OnContextsLoaded();
            RequestBuilder = std::make_unique<TMegamindRequestBuilder>(
                GetRequestPhase(),
                Config,
                std::move(AppHostParams),
                *SessionCtx,
                *RequestCtx,
                *ContextLoadResponse,
                SpeakerService,
                AliceLoggerOptions,
                LogContext
            );
            LogContext.LogEvent(NEvClass::DebugMessage("MegamindClient is under construction..."));
            MegamindClient = TMegamindClient::Create(
                GetRequestPhase(),
                Config,
                *SessionCtx,
                LogContext
            );
            LogContext.LogEvent(NEvClass::DebugMessage("MegamindClient constructed"));
        }
    }

    // specific steps for each phase
    if (!ProcessNextInput()) {
        LogContext.LogEventInfoCombo<NEvClass::NoMoreInputFromAppHostNeeded>();

        if (!FinalSubrequestSended) {
            // If we are here it is very likely that we have empty eou or asr server error,
            // but there may be some other errors
            // So, the only thing we can do here is to abort the current request
            SafeFlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterNoMoreInputNeededWithoutFinalRequest>();
        } else {
            // No more input, but we have to wait for megamind's response to the final request
        }

        return;
    }

    // continue consuming input
    SubscribeInput("continue consuming input");
}

void TMegamindServant::SubscribeInput(const TString& comment) {
    LogContext.LogEvent(NEvClass::DebugMessage(comment));
    AhContext().NextInput().Apply([servant = TIntrusivePtr<TMegamindServant>(this)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            servant->LogContext.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();

            if (!servant->FinalSubrequestSended || servant->AhContext().IsCancelled()) {
                servant->OnCancelRequest();

                // Here we can have a race with MegamindClient thread, so use mutex
                if (servant->AhContext().IsCancelled()) {
                    servant->SafeFlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterCancelFromAppHost>();
                } else {
                    servant->SafeFlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterAppHostEmptyInputWithoutFinalRequest>();
                }
            }
            return;
        }
        servant->OnNextInputSafe();
    });
}

bool TMegamindServant::CanSendRequest() const {
    return !FinalSubrequestSended && MegamindClient;
}

void TMegamindServant::SendRequest(const TSubrequestPtr& subrequest) {
    Y_ASSERT(!FinalSubrequestSended && "Only one final request allowed!");
    Y_ASSERT(MegamindClient && "MegamindClient is not built yet!");

    if (subrequest->Final) {
        FinalSubrequestSended = true;
        LogContext.LogEvent(NEvClass::InfoMessage("FinalSubrequestSended"));
        DLOG("FinalSubrequestSended = true");
    }

    if (NoVinsRequests_) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("has no_vins_requests experiment, use fake(empty) vins response");
        NAliceProtocol::TMegamindResponse response;
        response.MutableProtoResponse()->MutableResponse();
        response.SetRawJsonResponse("{\"response\":{}}");
        OnSubrequestResponse(*subrequest, response);
        return;
    }

    NJson::TJsonValue sessionLogValue;
    try {
        auto rtLogChild = MakeAtomicShared<TRTLogActivation>();
        if (LogContext.RtLog()) {
            *rtLogChild = TRTLogActivation(LogContext.RtLogPtr(), subrequest->SetraceLabel, /*newRequest=*/false);
        }

        int asrPartialNumber = -1;
        if (subrequest->PartialNumbers.AsrPartialNumber.Defined()) {
            asrPartialNumber = subrequest->PartialNumbers.AsrPartialNumber.GetRef();
        }

        if (subrequest->Request.HasRequest() && subrequest->Request.GetRequest().has_event()) {
            bool hasBiometryScore = subrequest->Request.GetRequest().event().HasBiometryScoring();
            Metrics.PushRate(
                subrequest->Final ? "mm_request_final" : "mm_request",
                hasBiometryScore ? "exist" : "missed",
                "bio_score");

            bool hasBiometryClassification = subrequest->Request.GetRequest().event().HasBiometryClassification();
            Metrics.PushRate(
                subrequest->Final ? "mm_request_final" : "mm_request",
                hasBiometryClassification ? "exist" : "missed",
                "bio_classification");
        }

        MegamindClient->SendRequest(RequestBuilder->Build(subrequest->Request, *rtLogChild, sessionLogValue, subrequest->Final), asrPartialNumber, rtLogChild)
            .Subscribe([
                self = TIntrusivePtr<TMegamindServant>(this),
                req = TSubrequestPtr(subrequest)
            ] (const NThreading::TFuture<NAliceProtocol::TMegamindResponse>& fut) {
                try {
                    Y_ASSERT(fut.HasValue() || fut.HasException());
                    DLOG("TMegamindServant::got response");
                    if (fut.HasException()) {
                        TString error = "unknown fut exception";
                        TString errorCode = "unknown1";
                        // add System.EventException directive
                        try {
                            fut.TryRethrow();
                        } catch (const TErrorWithCode &exc) {
                            error = exc.Error();
                            errorCode = exc.Code();
                            self->LogContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "Got exception from Megamind client: " << exc.Text()));
                        } catch (...) {
                            error = CurrentExceptionMessage();
                            self->LogContext.LogEvent(NEvClass::WarningMessage(TStringBuilder() << "Got exception from Megamind client: " << error));
                        }
                        self->OnSubrequestError(*req, error, errorCode);
                    } else {
                        // add response protobuf
                        const auto &response = fut.GetValueSync();
                        self->OnSubrequestResponse(*req, response);
                    }
                } catch (...) {
                    self->OnInternalError(CurrentExceptionMessage(), "error_on_error");
                }
            }
        );

    } catch (...) {
        static const TString failSendRequest{"fail_send_request"};
        Metrics.PushRate("request", "failed");
        const TErrorWithCode ec{
            failSendRequest,
            TStringBuilder{} << "Faild build MM request: " << CurrentExceptionMessage()
        };
        LogContext.LogEvent(NEvClass::ErrorMessage(ec.Text()));
        OnSubrequestError(*subrequest, ec.Error(), ec.Code());
    }

    {
        static const NVoicetech::NUniproxy2::TMessage fakeMsg;
        // sessionLogValue["EffectiveVinsUrl"] filled in MegamindClient->SendRequest
        // sessionLogValue["Body"]; filled in MegamindClient->SendRequest, but MUST BE CENSORED !!!
        NVoicetech::NUniproxy2::CensorMessage(fakeMsg,  NVoicetech::NUniproxy2::CFNone, sessionLogValue["Body"]);
        // SESSION_LOG with VinsRequest record
        SendSessionLogDirective(std::move(sessionLogValue), "VinsRequest");
    }
    DLOG("TMegamindServant::request was sent");
    static const TString requestWasSent{"Request was sent"};
    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(requestWasSent);
}

void TMegamindServant::OnError(const TString& error, const TString& errorCode) {
    TString text = TStringBuilder() << "code=" << errorCode << ": " << error;
    LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(std::move(text));
    if (!RequestCtx) {
        OnInternalError(TStringBuilder() << "TMegamindServant::OnError: not has RequestCtx on error: " << error, errorCode);
        return;
    }

    auto directive = CreateEventExceptionEx(RequestPhaseNode(GetRequestPhase()), errorCode, error, RequestCtx->GetHeader().GetMessageId());
    LogContext.LogEventErrorCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
    with_lock(MutexWriteToContext) {
        AhContext().AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
        FlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterSendToAppHostErrorDirective>();
    }
}

void TMegamindServant::OnInternalError(const TString& error, const TString& errorCode) {
    LogContext.LogEventErrorCombo<NEvClass::InternalError>(TStringBuilder() << "code=" << errorCode << ": " << error);
    if (RequestCtx) {
        auto directive = CreateEventExceptionEx(RequestPhaseNode(GetRequestPhase()), errorCode, error, RequestCtx->GetHeader().GetMessageId());
        LogContext.LogEventErrorCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
        with_lock(MutexWriteToContext) {
            AhContext().AddProtobufItem(directive, ITEM_TYPE_DIRECTIVE);
            IntermediateFlush<NEvClass::IntermediateFlushAppHostContextAfterSendToAppHostInternalErrorDirective>();
        }
    }
    with_lock(MutexWriteToContext) {
        RequestHandler.SetException(std::make_exception_ptr(error));
    }
}

void TMegamindServant::OnSubrequestError(TSubrequest& req, const TString& error, const TString& errorCode) {
    bool finalResponse = false;
    with_lock(req.Mutex) {
        finalResponse = req.Final;

        // Partials request might be dropped by the RPS limiters to decrease the Megamind load.
        // In that case the 429 http code will be returned.
        // Since it's an expected behaviour, we should not propagate this error to the client.
        // All other error codes as well as 429 on final request should be considered as real errors.
        // TODO @aradzevich: VOICESERV-4296 to rework
        if (finalResponse || errorCode != "http.429"sv) {
            req.Error = error;
            req.ErrorCode = errorCode;
            if (!req.ErrorCode) {
                req.ErrorCode = "unknown";
            }
        }

        req.Finish = TInstant::Now();
    }
    if (finalResponse) {
        OnHasFinalResult(req);
    }
}

void TMegamindServant::OnSubrequestResponse(TSubrequest& req, const NAliceProtocol::TMegamindResponse& response) {
    auto respPtr = MakeHolder<NAliceProtocol::TMegamindResponse>(response);
    bool finalResponse = false;
    with_lock(req.Mutex) {
        finalResponse = req.Final;
        req.Response.Swap(respPtr);
        req.Finish = TInstant::Now();
    }
    if (finalResponse) {
        LogContext.LogEventInfoCombo<NEvClass::OnFinalSubrequestResponse>(response.ShortUtf8DebugString());
        OnHasFinalResult(req);
    }
    return;
}


void TMegamindServant::TryAddSingleContextSaveRequest(
    const NAliceProtocol::TMegamindResponse& mmResponse,
    const TStringBuf contextSaveRequestItemType,
    const TStringBuf contextSaveNeedFullIncomingAudioFlag,
    const bool isImportant
) {
    NAlice::NCuttlefish::NContextSaveClient::TContextSaveStarter contextSaveStarter;
    const uint32_t requestSize = contextSaveStarter.AddDirectives(
        mmResponse.GetProtoResponse(),
        [isImportant](const NAlice::NSpeechKit::TDirective& directive) {
            return IsDirectiveImportant(directive) == isImportant;
        },
        [isImportant](const NAlice::NSpeechKit::TProtobufUniproxyDirective& directive) {
            Y_UNUSED(directive);

            // TODO(VOICESERV-4205): Replace "false" with normal condition
            return false == isImportant;
        }
    );

    if (0 < requestSize) {
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostContextSaveRequest>(
            contextSaveStarter.GetRequestRef().ShortUtf8DebugString(),
            isImportant
        );

        std::move(contextSaveStarter).Finalize(
            AhContext(),
            contextSaveRequestItemType,
            contextSaveNeedFullIncomingAudioFlag
        );
    }
}

void TMegamindServant::TryAddContextSaveRequest(const NAliceProtocol::TMegamindResponse& response) {
    TryAddSingleContextSaveRequest(
        response,
        ITEM_TYPE_CONTEXT_SAVE_REQUEST,
        EDGE_FLAG_CONTEXT_SAVE_NEED_FULL_INCOMING_AUDIO,
        /* isImportant = */ false
    );
    TryAddSingleContextSaveRequest(
        response,
        ITEM_TYPE_CONTEXT_SAVE_IMPORTANT_REQUEST,
        EDGE_FLAG_CONTEXT_SAVE_IMPORTANT_NEED_FULL_INCOMING_AUDIO,
        /* isImportant = */ true
    );
}

void TMegamindServant::TryAddSessionSaveRequest(const NAliceProtocol::TMegamindResponse& response) {
    for (const auto& kv : response.GetProtoResponse().GetSessions()) {
        const TString data = Base64Decode(kv.second);
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
            TStringBuilder()
                << "We will pass new megamind session (md5 hash "
                << MD5::Calc(data) << ") to context_save"
        );

        NCachalotProtocol::TMegamindSessionRequest req;
        auto& storeReq = *req.MutableStoreRequest();
        if (SessionCtx->HasUserInfo()) {
            const auto& info = SessionCtx->GetUserInfo();
            if (info.HasVinsApplicationUuid()) {
                storeReq.SetUuid(info.GetVinsApplicationUuid());
            } else if (info.HasUuid()) {
                storeReq.SetUuid(info.GetUuid());
            }
            if (info.HasPuid()) {
                storeReq.SetPuid(info.GetPuid());
            }
        }
        storeReq.SetDialogId(kv.first);
        storeReq.SetRequestId(RequestCtx->GetHeader().GetReqId());
        {
            storeReq.SetData("<censored>");  // MUST BE overwritten later!
            LogContext.LogEvent<NEvClass::SendToAppHostMegamindSessionRequest>(req.ShortUtf8DebugString());
        }
        storeReq.SetData(data);
        if (RequestCtx->GetSettingsFromManager().GetCacheMegamindSessionCrossDc()) {
            storeReq.SetLocation(TInstanceTags::Get().Geo);
        }
        with_lock(MutexWriteToContext) {
            const TStringBuf uuid = req.GetStoreRequest().GetUuid();
            AhContext().AddBalancingHint("CACHALOT_MM_SESSION", CityHash64(uuid.data(), uuid.size()));
            AhContext().AddProtobufItem(req, ITEM_TYPE_MEGAMIND_SESSION_REQUEST);
        }
    }
}

void TMegamindServant::TrySetupExperiments() {
    if (!Request || !RequestCtx) {
        return;
    }

    // apply experiments from RequestContext
    for (auto& [k, v]: RequestCtx->GetExpFlags()) {
        (*Request->MutableRequestBase()->MutableRequest()->MutableExperiments()->MutableStorage())[k].SetString(v);
    }
}

void TMegamindServant::ProcessUsedMegamindResponse(const NAliceProtocol::TMegamindResponse& response) {
    // directives
    static const TString saveVoiceprint = "save_voiceprint";
    static const TString removeVoiceprint = "remove_voiceprint";

    const auto& proto = response.GetProtoResponse();
    for (const auto& directive : proto.GetVoiceResponse().GetDirectives()) {
        if (directive.GetType() != UNIPROXY_ACTION_DIRECTIVE_TYPE) {
            LogContext.LogEventInfoCombo<NEvClass::DebugMessage>(TStringBuilder() << "ignore here directive type=" << directive.GetType() << " name=" << directive.GetName());
            continue;
        }

        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "process uniproxy_action directive=" << directive.GetName());
        // process here megamind directives (create voiceprint from enrollings, etc)
        if (directive.GetName() == saveVoiceprint) {
            AddUserToBioContext(directive);
        } else if (directive.GetName() == removeVoiceprint) {
            RemoveUserFromBioContext(directive);
        } else {
            LogContext.LogEventInfoCombo<NEvClass::DebugMessage>(TStringBuilder() << "ignore2 here directive type=" << directive.GetType() << " name=" << directive.GetName());
        }
        //TODO: process force_interruption_spotter?!:
        // for it we need switch from hack with making TTS.Generate from mm_response (at WS_ADAPTER_OUT) to generate tts_request right here
        // DO NOT FORGET TO IMPLEMENT THE EMOTION HACK FROM VOICESERV-4018
    }
}

void TMegamindServant::AddUserToBioContext(const NSpeechKit::TDirective& directive) {
    using TSaveVoiceprintDirective = NAlice::NScenarios::TSaveVoiceprintDirective;
    static auto enrich = [](NAliceProtocol::TBioContextUpdate& bioUpdate, TSaveVoiceprintDirective&& saveVoiceprint) {
        bioUpdate.MutableCreateUser()->Swap(&saveVoiceprint);
    };

    AddBioContextUpdate<TSaveVoiceprintDirective>(directive, enrich);
}

void TMegamindServant::RemoveUserFromBioContext(const NSpeechKit::TDirective& directive) {
    using TRemoveVoiceprintDirective = NAlice::NScenarios::TRemoveVoiceprintDirective;
    static auto enrich = [](NAliceProtocol::TBioContextUpdate& bioUpdate, TRemoveVoiceprintDirective&& removeVoiceprint) {
        bioUpdate.MutableRemoveUser()->Swap(&removeVoiceprint);
    };

    AddBioContextUpdate<TRemoveVoiceprintDirective>(directive, enrich);
}

template <typename TTypedDirective, typename TEnrich>
void TMegamindServant::AddBioContextUpdate(const NSpeechKit::TDirective& directive, TEnrich enrich) {
    if (!directive.HasPayload()) {
        throw TInvalidMegamindDirective() << "no payload in save_voiceprint";
    }

    NAliceProtocol::TBioContextUpdate bioUpdate;
    enrich(bioUpdate, NAlice::StructToMessage<TTypedDirective>(directive.GetPayload()));

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostBioContextUpdate>(bioUpdate.ShortUtf8DebugString());
    with_lock(MutexWriteToContext) {
        AhContext().AddProtobufItem(bioUpdate, ITEM_TYPE_REQUEST_BIO_CONTEXT_UPDATE);
    }
}

void TMegamindServant::AddRequestDebugInfo(NAliceProtocol::TRequestDebugInfo&& debugInfo) {
    NAliceProtocol::TDirective directive;
    *directive.MutableRequestDebugInfo() = std::move(debugInfo);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
    with_lock(MutexWriteToContext) {
        AhContext().AddProtobufItem(directive, NCuttlefish::ITEM_TYPE_DIRECTIVE);
    }
}

void TMegamindServant::AddTtsRequestIfNeed(const NAliceProtocol::TMegamindResponse& response) {
    TString text = response.GetProtoResponse().GetVoiceResponse().GetOutputSpeech().GetText();
    if (!text) {
        return;
    }
    const auto& requestCtx = RequestCtx.GetRef();

    // st.yandex-team.ru/VOICESERV-3571#601bd9527f4dd226ad80ce42
    if (NExpFlags::ConductingExperiment(requestCtx, "silent_vins")) {
        Metrics.PushRate("tts_with_silent_vins");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Skip tts generation due to 'silent_vins' exp");
        return;
    }

    NVoicetech::NUniproxy2::TMessage ttsMsg = CreateTtsGenerate(requestCtx, response, requestCtx.GetHeader().GetMessageId());
    NAlice::NTts::NProtobuf::TRequest ttsRequest;
    TtsGenerateMessageToTtsRequest(ttsRequest, requestCtx, SessionCtx.GetRef(), ttsMsg);
    LogContext.LogEvent(NEvClass::SendToAppHostTtsRequest(GetCensoredTtsRequestStr(ttsRequest), /* isFinal= */ true));
    with_lock(MutexWriteToContext) {
        // Flag for any input graph edge expressions
        AhContext().AddFlag(EDGE_FLAG_TTS);
        AhContext().AddProtobufItem(ttsRequest, NCuttlefish::ITEM_TYPE_TTS_REQUEST);
    }
}

void TMegamindServant::AddMegamindRunReady() {
    with_lock(MutexWriteToContext) {
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMegamindRunReady>();
        AhContext().AddProtobufItem(NAliceProtocol::TMegamindRunReady(), NCuttlefish::ITEM_TYPE_MEGAMIND_RUN_READY);
        FlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterFinalMegamindRunResponse>();
    }
}

void TMegamindServant::SendSessionLogDirective(NJson::TJsonValue&& sessionLogValue, const TString& type) {
    try {  // SESSION_LOG with record
        NAliceProtocol::TUniproxyDirective directive;
        auto& sessionLog = *directive.MutableSessionLog();
        sessionLogValue["type"] = type;
        sessionLogValue["ForEvent"] = RequestCtx->GetHeader().GetMessageId();
        sessionLog.SetName("Directive");
        sessionLog.SetValue(NJson::WriteJson(sessionLogValue, false, true));
        sessionLog.SetAction("response");
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostSessionLog>(
            TStringBuilder() << "ForEvent: " << RequestCtx->GetHeader().GetMessageId() << ", type: " << type
        );
        with_lock(MutexWriteToContext) {
            AhContext().AddProtobufItem(directive, NCuttlefish::ITEM_TYPE_UNIPROXY2_DIRECTIVE_SESSION_LOG);
            IntermediateFlush<NEvClass::IntermediateFlushAppHostContextAfterSendToAppHostSessionLog>();
            Metrics.PushRate("session_log", "sent");
        }
    } catch (...) {
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "fail build SessionLog/" << type << ": " << CurrentExceptionMessage());
    }
}

template <typename TFlushAndFinishEvent>
void TMegamindServant::FlushAndFinish() {
    if (!AhContextFlushed) {
        LogContext.LogEventInfoCombo<TFlushAndFinishEvent>();
        AhContextFlushed = true;
        AhContext().Flush();
        RequestHandler.Finish();
    }
}

template <typename TFlushAndFinishEvent>
void TMegamindServant::SafeFlushAndFinish() {
    with_lock(MutexWriteToContext) {
        FlushAndFinish<TFlushAndFinishEvent>();
    }
}

template <typename TIntermediateFlushEvent>
void TMegamindServant::IntermediateFlush() {
    LogContext.LogEventInfoCombo<TIntermediateFlushEvent>();
    AhContext().IntermediateFlush();
}

template <typename TIntermediateFlushEvent>
void TMegamindServant::SafeIntermediateFlush() {
    with_lock(MutexWriteToContext) {
        IntermediateFlush<TIntermediateFlushEvent>();
    }
}

// ------------------------------------------------------- TMegamindRunServant

ERequestPhase TMegamindRunServant::GetRequestPhase() const {
    return ERequestPhase::RUN;
}

void TMegamindRunServant::OnCreate() {
    LogContext.LogEvent(NEvClass::MegamindRunAdapter());
    StateMachine_.Process(NSM::TStartEv());
}

void TMegamindRunServant::OnContextsLoaded() {
    StateMachine_.Process(NMegamindEvents::TContextsLoaded(RequestCtx->GetHeader().GetFullName()));
}

void TMegamindRunServant::OnReceiveRequestContext(const NAliceProtocol::TRequestContext& reqCtx) {
    TMegamindServant::OnReceiveRequestContext(reqCtx);
    if (reqCtx.HasHeader()) {
        auto& header = reqCtx.GetHeader();
        VoiceInput_ = header.HasFullName() && header.GetFullName() == TStringBuf("vins.voiceinput");
    }

    auto& settingsFromManager = RequestCtx->GetSettingsFromManager();

    if (NExpFlags::ConductingExperiment(RequestCtx.GetRef(), "disable_partials")) {
        UseAsrPartials_ = false;
    } else if (NExpFlags::ConductingExperiment(RequestCtx.GetRef(), "enable_partials")) {
        UseAsrPartials_ = true;
    } else if (settingsFromManager.HasUseMMPartials()) {
        UseAsrPartials_ = settingsFromManager.GetUseMMPartials();
    }
    if (NExpFlags::ConductingExperiment(RequestCtx.GetRef(), "no_vins_requests")) {
        NoVinsRequests_ = true;
    }
    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "using_asr_partials " << (UseAsrPartials_ ? "enabled" : "disabled"));
}

bool TMegamindRunServant::ProcessNextInput() {
    if (!Request && AhContext().HasProtobufItem(ITEM_TYPE_MEGAMIND_REQUEST)) {
        Request = AhContext().GetOnlyProtobufItem<NAliceProtocol::TMegamindRequest>(ITEM_TYPE_MEGAMIND_REQUEST);
        StateMachine_.Process(NMegamindEvents::TReceiveMmRequest());
        NAliceProtocol::TMegamindRequest req{*Request};
        Censore(req);
        TString logText{req.ShortUtf8DebugString()};
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostMegamindRequest>(logText);
        DLOG("RecvFromAppHostMegamindRequest: " << logText);
        TrySetupExperiments();
    }
    size_t newAsrResponsesCount = 0;
    if (!HasEou_) {
        // << after processing EOU we can has thread race on access to BiometryScoring/BiometryClassification from here and SendSubrequestForAsrResponse()
        {
            auto items = AhContext().GetProtobufItemRefs(ITEM_TYPE_YABIO_PROTO_RESPONSE, NAppHost::EContextItemSelection::Input);
            for (auto it = items.begin(); it != items.end(); ++it) {
                try {
                    NYabio::TResponse response;
                    ParseProtobufItem(*it, response);
                    ProcessYabioResponse(response);
                } catch (...) {
                    LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "fail process yabio_response: " << CurrentExceptionMessage());
                    //TODO: add increment metric (for detect problems)
                }
            }
        }

        auto items = AhContext().GetProtobufItemRefs(ITEM_TYPE_ASR_PROTO_RESPONSE, NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end(); ++it) {
            AsrEngineResponseProtobuf::TASRResponse asrResponse;
            try {
                ParseProtobufItem(*it, asrResponse);
            } catch (...) {
                OnError(TStringBuilder() << "fail parse asr_proto_response: " << CurrentExceptionMessage(), "parse_asr_resp");
                return false;
            }
            {
                TString logText{asrResponse.ShortUtf8DebugString()};
                LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostAsrResponse>(logText);
                DLOG("RecvFromAppHostAsrResponse: " << logText);
            }
            if (asrResponse.HasAddDataResponse()) {
                auto& addDataResponse = asrResponse.GetAddDataResponse();
                if (addDataResponse.GetIsOk()) {
                    if (addDataResponse.GetResponseStatus() == AsrEngineResponseProtobuf::Active) {
                        StateMachine_.UpdateLastPartialAsrResultTiming();
                        if (!HasFirstAsrPartial_) {
                            HasFirstAsrPartial_ = true;
                            StateMachine_.Process(NMegamindEvents::TFirstPartialAsrResult());
                        }
                        if (!HasFirstNotEmptyAsrPartial_ && GetAddDataResponseWords(addDataResponse)) {
                            HasFirstNotEmptyAsrPartial_ = true;
                            StateMachine_.Process(NMegamindEvents::TNotEmptyPartialAsrResult());
                        }
                        if (GetAddDataResponseWords(addDataResponse)) {
                            newAsrResponsesCount += 1;
                            AsrResponses_.emplace_back(std::move(asrResponse));
                        }
                    } else if (addDataResponse.GetResponseStatus() == AsrEngineResponseProtobuf::EndOfUtterance) {
                        bool emptyResult = !GetAddDataResponseWords(addDataResponse);
                        StateMachine_.Process(NMegamindEvents::TReceiveEouAsrResult(/*empty=*/emptyResult));
                        // on empty eou result simply finish servant
                        if (emptyResult) {
                            static const TString logText{"got empty EOU result, finish here"};
                            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(logText);
                            return false;
                        }
                        newAsrResponsesCount += 1;
                        AsrResponses_.emplace_back(std::move(asrResponse));
                    }
                } else {
                    {
                        TString logText{TStringBuilder() << "asr responses stream broken: "
                            << (addDataResponse.HasErrMsg() ? "error unknown" : addDataResponse.GetErrMsg())};
                        LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(logText);
                    }
                    return false;  // we can't cancel currently executed requests, so simply finish servant (return without call NextInput())
                }
            }
        }
    }

    if (CanSendRequest() && Request) {
        if (VoiceInput_) {
            if (RequestCtx->GetPredefinedResults().GetAsr()) {
                // asr result already placed in base mm_request
                TSubrequestPtr req(new TSubrequest);
                req->Final = true;
                req->Request = Request->GetRequestBase();
                req->Request.MutableHeader()->SetRefMessageId(RequestCtx->GetHeader().GetMessageId());
                req->Request.MutableRequest()->MutableEvent()->SetType(NAlice::voice_input);
                {
                    TStringOutput so(req->SetraceLabel);
                    so << " (noasr)voice_input:";
                    auto& asrResults = Request->GetRequestBase().GetRequest().GetEvent().GetAsrResult();
                    for (auto& result : asrResults) {
                        for (auto& w : result.GetWords()) {
                            so << " " << w.GetValue();
                        }
                        break;
                    }
                }
                StateMachine_.Process(NMegamindEvents::TReceiveEouAsrResult(/*empty=*/false));
                StateMachine_.Process(NMegamindEvents::TSendSubrequest());
                StateMachine_.Process(NMegamindEvents::TUseEouAsrResult());
                SendRequest(req);
            } else {
                while (AsrResponses_.size()) {
                    //TODO: optimize here (if already has eou, use/process only this response)
                    ProcessAsrResponse(AsrResponses_.front(), /*postponed=*/ AsrResponses_.size() > newAsrResponsesCount);
                    AsrResponses_.pop_front();
                }
            }
        } else {
            TSubrequestPtr req(new TSubrequest);
            req->Final = true;
            req->Request = Request->GetRequestBase();
            req->Request.MutableHeader()->SetRefMessageId(RequestCtx->GetHeader().GetMessageId());
            SetSetraceLabel(req);
            StateMachine_.Process(NMegamindEvents::TSendSubrequest());
            SendRequest(req);
        }
    }
    return true;
}


bool GetAsrPartialsThresholdFromFlags(
    double* threshold,
    NVoice::NExperiments::TFlagsJsonFlagsConstRef flags,
    TLogContext logContext
) {
    if (const TMaybe<double> thresholdValue = flags.GetExperimentValueFloat("asr_partials_threshold")) {
        *threshold = thresholdValue.GetRef();
        return true;
    }

    if (const TMaybe<TString> thresholdValueStr = flags.GetValueFromName("asr_partials_threshold_")) {
        try {
            *threshold = FromString<double>(thresholdValueStr.GetRef());
            return true;
        } catch (...) {
            logContext.LogEventInfoCombo<NEvClass::TInfoMessage>(TStringBuilder()
                << "fail extract float from uaas flag=asr_partials_threshold_" << thresholdValueStr
                << ": " << CurrentExceptionMessage());
        }
    }

    return false;
}

void TMegamindRunServant::ProcessAsrResponse(const TReceivedAsrResponse& receivedAsrResponse, bool postponed) {
    if (!AsrPartialNumber.Defined()) {
        AsrPartialNumber = 0;
    } else {
        AsrPartialNumber.GetRef()++;
    }
    if (HasEou_) {
        return;
    }

    auto& asrResponse = receivedAsrResponse.AsrResponse;
    if (!asrResponse.HasAddDataResponse()) {
        return;
    }

    auto& addDataResponse = asrResponse.GetAddDataResponse();
    auto& settingsFromManager = RequestCtx->GetSettingsFromManager();
    HasEou_ = addDataResponse.GetResponseStatus() == AsrEngineResponseProtobuf::EndOfUtterance;

    if (!HasEou_ && !UseAsrPartials_) {
        return;
    }

    if (!HasEou_) {
        double threshold = 0;
        if (settingsFromManager.HasAsrResultThreshold()) {
            threshold = settingsFromManager.GetAsrResultThreshold();
        }

        // Rewrite its setting for asr_partials_threshold with value from AB experiment.
        if (
            ContextLoadResponse.Defined() && ContextLoadResponse->HasFlagsInfo() &&
            GetAsrPartialsThresholdFromFlags(
                &threshold,
                NVoice::NExperiments::TFlagsJsonFlagsConstRef(&(ContextLoadResponse->GetFlagsInfo())),
                LogContext
            )
        ) {
            LogContext.LogEventInfoCombo<NEvClass::TInfoMessage>(TStringBuilder()
                << "asr_partials_threshold extracted from context_load response: " << threshold);
        }

        double thrownPartialsFraction = 1;
        if (addDataResponse.GetRecognition().HasThrownPartialsFraction()) {
            thrownPartialsFraction = addDataResponse.GetRecognition().GetThrownPartialsFraction();
        }
        if (thrownPartialsFraction < threshold) {
            NJson::TJsonValue sessionLogValue;
            auto& body = sessionLogValue["Body"];
            body["threshold"] = threshold;
            body["thrown_partials_fraction"] = thrownPartialsFraction;
            SendSessionLogDirective(std::move(sessionLogValue), "SkipAsrPartial");
            return;
        }
    }
    if (!HasEou_) {
        double droppedPartialFraction = 0;
        if (settingsFromManager.HasAsrDroppedPartialsFraction()) {
            droppedPartialFraction = settingsFromManager.GetAsrDroppedPartialsFraction();
        }
        if (droppedPartialFraction > 0 && RandomNumber<double>() < droppedPartialFraction) {
            NJson::TJsonValue sessionLogValue;
            auto& body = sessionLogValue["Body"];
            body["asr_dropped_partials_fraction"] = droppedPartialFraction;
            SendSessionLogDirective(std::move(sessionLogValue), "DropAsrPartial");
            return;
        }
    }

    // fill key& words
    TString key;
    if (addDataResponse.HasCacheKey()) {
        key = addDataResponse.GetCacheKey();
    }
    TString words = GetAddDataResponseWords(addDataResponse);
    // here we can has not recognition (ValidationFail as example)

    TSubrequestKey sKey(key, words, SpeakerService.GetActiveSpeaker().Get());
    auto res = Subrequests_.try_emplace(sKey, TSubrequestPtr());
    TSubrequestPtr& subrequest = res.first->second;  // nullptr if created new TSubrequest
    if (subrequest) {
        if (!HasEou_) {
            return; // already has subrequest for this partial - nothing need TODO
        }

        // already has partial subrequest for this asr result, update it to Final & try use it
        FinalSubrequestSended = true;  // retroactively mark sending final request (for fine log)
        bool hasResult = false;
        bool hasTrash = false;
        with_lock(subrequest->Mutex) {
            subrequest->Final = true;
            if (addDataResponse.HasNumber()) {
                subrequest->PartialNumbers.AsrPartialNumber = addDataResponse.GetNumber();
            } else {
                subrequest->PartialNumbers.AsrPartialNumber = AsrPartialNumber;
            }
            hasResult = bool(subrequest->Error);
            if (!hasResult) {
                if (subrequest->Response) {
                    if (subrequest->Response->GetProtoResponse().GetVoiceResponse().GetIsTrashPartial()) {
                        hasTrash = true;
                    } else {
                        hasResult = true;
                    }
                } else {
                    subrequest->SpareAddDataResponse = addDataResponse;
                }
            }
        }
        if (!hasTrash) {
            StateMachine_.Process(NMegamindEvents::TUseEouAsrResult(postponed, subrequest, hasResult));
            if (hasResult) {
                StateMachine_.SetUsefulPartialOk();
                SendLegacySessionLogPartial("good_partial");
                OnHasFinalResult(*subrequest);
            } // else: not finished request marked as Final - so when request will be finished, it result be processed using OnHasFinalResult
            return;
        }

        // this subrequest result is useless, need new subrequest
        StateMachine_.SetUsefulPartialTrashedByMM();
        SendLegacySessionLogPartial("trash_partial");
    } else {
        if (HasEou_ && VoiceInput_) {
            StateMachine_.SetUsefulPartialNotFound();
            SendLegacySessionLogPartial("bad_partial");
        } else {
            DLOG("run partial");
        }
    }
    subrequest.Reset(new TSubrequest);
    subrequest->ReceiveAsrResult = receivedAsrResponse.Received;
    subrequest->Postponed = postponed;
    subrequest->Final = HasEou_;
    subrequest->PartialNumbers.AsrPartialNumber = AsrPartialNumber;
    SetSetraceLabel(subrequest, words);
    {
        // enrich setrace backend label with asr result text (+cache_key)
        TStringOutput so(subrequest->SetraceLabel);
        so << " (";
        if (HasEou_) {
            so << " eou=true ";
        }
        if (key) {
            so << " cache_key=" << key << ' ';
        }
        if (auto activeSpeaker = SpeakerService.GetActiveSpeaker(); activeSpeaker != nullptr) {
            so << " match=" << activeSpeaker->GuestUserOptions.GetPersId() << ' ';
        } else {
            so << " nomatch ";
        }
        so << ")";
    }
    if (HasEou_) {
        StateMachine_.Process(NMegamindEvents::TUseEouAsrResult(postponed));
    }
    SendSubrequestForAsrResponse(subrequest, addDataResponse);
}

void TMegamindRunServant::InitAndSendSubrequestForSpareAsrResult(const TSubrequest& baseSubrequest) {
    TSubrequestPtr subrequest{new TSubrequest};
    subrequest->ReceiveAsrResult = baseSubrequest.ReceiveAsrResult;
    subrequest->PartialNumbers = baseSubrequest.PartialNumbers;
    subrequest->Postponed = baseSubrequest.Postponed;
    subrequest->Final = true;
    subrequest->SetraceLabel = TString{"SPARE:"} + baseSubrequest.SetraceLabel;
    if (HasEou_) {
        StateMachine_.Process(NMegamindEvents::TUseEouAsrResult(baseSubrequest.Postponed));
    }
    SendSubrequestForAsrResponse(subrequest, baseSubrequest.SpareAddDataResponse.GetRef());
}

TString TMegamindRunServant::GetAddDataResponseWords(const AsrEngineResponseProtobuf::TAddDataResponse& addDataResponse) {
    TString words;
    if (addDataResponse.HasRecognition()) {
        auto& recognition = addDataResponse.GetRecognition();
        auto& hypos = recognition.GetHypos();
        if (hypos.size()) {
            TStringOutput so(words);
            for (auto& word : hypos[0].GetWords()) {
                if (words.Size()) {
                    so << ' ';
                }
                so << word;
            }
        }
    }
    return words;
}

void TMegamindRunServant::ProcessYabioResponse(const NYabio::TResponse& response) {
    {
        TString logText;
        if (response.HasAddDataResponse() && response.GetAddDataResponse().has_context() && response.GetAddDataResponse().context().enrolling().size()) {
            NYabio::TResponse reducedResponse(response);
            for (auto& e : *reducedResponse.MutableAddDataResponse()->mutable_context()->mutable_enrolling()) {
                e.clear_voiceprint();
            }
            logText = reducedResponse.ShortUtf8DebugString();
        } else {
            logText = response.ShortUtf8DebugString();
        }
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostYabioResponse>(logText);
    }

    static const TString ok = "ok";
    static const TString failed = "failed";
    if (!response.HasAddDataResponse()) {
        if (response.HasInitResponse()) {
            auto& initResponse = response.GetInitResponse();
            if (initResponse.GetresponseCode() != 200) {
                NAlice::TBiometryClassification biometryClassification;
                biometryClassification.SetStatus(failed);
                BiometryClassification = std::move(biometryClassification);
                const TString logText{TStringBuilder() << "got biometry init error: code=" << int(initResponse.GetresponseCode())};
                LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(logText);
            }
        }
        return;
    }

    auto& addDataResponse = response.GetAddDataResponse();
    if (!response.HasForMethod() || response.GetForMethod() == YabioProtobuf::Classify) {
        NAlice::TBiometryClassification biometryClassification;
        if (addDataResponse.GetresponseCode() != 200) {
            biometryClassification.SetStatus(failed);
            if (BiometryClassification && BiometryClassification->GetStatus() == ok) {
                // if has already store good result, continue use it (for robust)
                const TString logText{TStringBuilder() << "got biometry classification error: code="
                    << int(addDataResponse.GetresponseCode()) << ", ignore it & continue use data from last success response: code="
                };
                LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(logText);
            } else {
                int partialNumber = 0;
                if (BiometryClassification && BiometryClassification->HasPartialNumber()) {
                    partialNumber = BiometryClassification->GetPartialNumber() + 1;
                }
                biometryClassification.SetPartialNumber(partialNumber);
                BiometryClassification = std::move(biometryClassification);
            }
            return;
        }

        biometryClassification.SetStatus(ok);
        for (auto& classificationSimple : addDataResponse.GetclassificationResults()) {
            auto& simple = *biometryClassification.AddSimple();
            if (classificationSimple.has_tag()) {
                simple.SetTag(classificationSimple.tag());
            }
            if (classificationSimple.has_classname()) {
                simple.SetClassName(classificationSimple.classname());
            }
        }
        for (auto& classification : addDataResponse.Getclassification()) {
            auto& scores = *biometryClassification.AddScores();
            scores.SetClassName(classification.classname());
            scores.SetConfidence(classification.confidence());
            if (classification.has_tag()) {
                scores.SetTag(classification.tag());
            }
        }
        if (biometryClassification.GetSimple().size() || biometryClassification.GetScores().size()) {
            // got fresh & not empty result, - store it
            int partialNumber = 0;
            if (BiometryClassification && BiometryClassification->HasPartialNumber()) {
                partialNumber = BiometryClassification->GetPartialNumber() + 1;
            }
            biometryClassification.SetPartialNumber(partialNumber);
            BiometryClassification = std::move(biometryClassification);
        }
    } else if (response.GetForMethod() == YabioProtobuf::Score) {
        NAlice::TBiometryScoring biometryScoring;
        if (addDataResponse.GetresponseCode() != 200) {
            biometryScoring.SetStatus(failed);
            if (BiometryScoring && BiometryScoring->GetStatus() == ok) {
                // if has already store good result, continue use it (for robust)
                const TString logText{TStringBuilder() << "got biometry scoring error: code="
                    << int(addDataResponse.GetresponseCode()) << ", ignore it & continue use data from last success response: code="
                };
                LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(logText);
            } else {
                int partialNumber = 0;
                if (BiometryScoring && BiometryScoring->HasPartialNumber()) {
                    partialNumber = BiometryScoring->GetPartialNumber() + 1;
                }
                biometryScoring.SetPartialNumber(partialNumber);
                BiometryScoring = std::move(biometryScoring);
                MaxBiometryScore.Clear();
            }
            return;
        }

        biometryScoring.SetStatus(ok);
        if (response.HasGroupId()) {
            biometryScoring.SetGroupId(response.GetGroupId());
        }
        if (addDataResponse.has_context()) {
            auto& context = addDataResponse.context();
            if (context.has_group_id()) {
                biometryScoring.SetGroupId(context.group_id());
            }
            for (auto& enrolling : context.enrolling()) {
                biometryScoring.SetRequestId(enrolling.request_id());
            }
        }
        double maxScore = -1;
        for (auto& x : addDataResponse.scores_with_mode()) {
            auto& sm = *biometryScoring.AddScoresWithMode();
            sm.SetMode(x.mode());
            auto& score = *sm.AddScores();
            for (auto& s : x.scores()) {
                score.SetUserId(s.user_id());
                score.SetScore(s.score());
                if (s.score() > maxScore) {
                    maxScore = s.score();
                }
            }
        }
        if (biometryScoring.GetScores().size() || biometryScoring.GetScoresWithMode().size() || biometryScoring.HasRequestId()) {
            // got fresh & not empty result, - store it
            int partialNumber = 0;
            if (BiometryScoring && BiometryScoring->HasPartialNumber()) {
                partialNumber = BiometryScoring->GetPartialNumber() + 1;
            }
            biometryScoring.SetPartialNumber(partialNumber);
            BiometryScoring = std::move(biometryScoring);
            if (maxScore >= 0) {
                MaxBiometryScore = maxScore;
            }
        }
    }
}

void TMegamindRunServant::SendSubrequestForAsrResponse(TSubrequestPtr req, const AsrEngineResponseProtobuf::TAddDataResponse& addDataResponse) {
    // update base request from addDataResponse
    req->Request = Request->GetRequestBase();
    req->Request.MutableHeader()->SetRefMessageId(RequestCtx->GetHeader().GetMessageId());
    auto& mmRequest = *req->Request.MutableRequest();
    auto& event = *mmRequest.MutableEvent();
    event.SetType(NAlice::voice_input);
    const bool finalSubrequest = addDataResponse.GetResponseStatus() == AsrEngineResponseProtobuf::EndOfUtterance;
    event.SetEndOfUtterance(finalSubrequest);

    if (addDataResponse.HasIsWhisper()) {
        const bool isWhisper = addDataResponse.GetIsWhisper();
        event.SetAsrWhisper(isWhisper);
    }
    req->Final = finalSubrequest;
    if (addDataResponse.HasNumber()) {
         event.SetHypothesisNumber(addDataResponse.GetNumber());
    }
    if (addDataResponse.HasRecognition()) {
        auto& recognition = addDataResponse.GetRecognition();
        auto& hypos = recognition.GetHypos();
        for (auto& hypo : hypos) {
            auto& asrResult = *event.AddAsrResult();
            asrResult.SetConfidence(hypo.GetTotalScore());
            if (hypo.HasNormalized()) {
                asrResult.SetNormalized(hypo.GetNormalized());
            }
            for (auto& word: hypo.GetWords()) {
                auto& mmWord = *asrResult.AddWords();
                mmWord.SetValue(word);
                // mmWord.SetConfidence(1.0);  // TODO:return?
            }
        }
    }
    if (addDataResponse.HasCoreDebug()) {
        event.SetAsrCoreDebug(addDataResponse.GetCoreDebug());
    }
    if (addDataResponse.HasDurationProcessedAudio()) {
        event.SetAsrDurationProcessedAudio(addDataResponse.GetDurationProcessedAudio());
    }
    if (SessionCtx->GetExperiments().GetFlagsJsonData().HasAppInfo()) {
        mmRequest.MutableAdditionalOptions()->SetAppInfo(SessionCtx->GetExperiments().GetFlagsJsonData().GetAppInfo());
    }

    // TODO: very later: if (addDataResponse.HasBioResult()) ...
    // TODO: later: event.SetMusicResult
    // mixin bio results from separate yabio (yabio_adapter)
    if (BiometryScoring) {
        if (BiometryScoring->HasPartialNumber()) {
            req->PartialNumbers.BioScoringPartialNumber = BiometryScoring->GetPartialNumber();
        }
        *event.MutableBiometryScoring() = *BiometryScoring;
        if (MaxBiometryScore.Defined()) {
            req->MaxBiometryScore = MaxBiometryScore.GetRef();
        }
    }
    if (BiometryClassification) {
        if (BiometryClassification->HasPartialNumber()) {
            req->PartialNumbers.BioClassificationPartialNumber = BiometryClassification->GetPartialNumber();
        }
        *event.MutableBiometryClassification() = *BiometryClassification;
    }
    StateMachine_.Process(NMegamindEvents::TSendSubrequest());
    SendRequest(req);
}

void TMegamindRunServant::OnHasFinalResult(TSubrequest& req) {
    // get response|error from mutex lock
    TMaybe<TString> error;
    TString errorCode;
    THolder<NAliceProtocol::TMegamindResponse> response;
    TInstant finish;
    with_lock(req.Mutex) {
        Y_ASSERT(req.Final && "call OnHasFinalResult not on final request");
        req.Error.Swap(error);
        errorCode = req.ErrorCode;
        req.Response.Swap(response);
        finish = req.Finish;
    }
    if (VoiceInput_) {  // handle here misc hacks/trash logic
        if (response && response->GetProtoResponse().GetVoiceResponse().GetIsTrashPartial()) {
            if (req.Request.GetRequest().GetEvent().GetEndOfUtterance()) {
                response.Reset();
                errorCode = "internal_mm_error";
                error = "got from MM IsTrashPartial response for request with EndOfUtterance=true";
            } else {
                if (req.SpareAddDataResponse.Defined()) {
                    SendLegacySessionLogPartial("bad_partial");
                    InitAndSendSubrequestForSpareAsrResult(req);
                } // else got thread race between ProcessAsrResponse and this method
                return;
            }
        }
    }

    if (req.MaxBiometryScore.Defined()) {
        StateMachine_.PushMaxBiometryScore(req.MaxBiometryScore.GetRef());
    }
    TString intentName = response ? GetIntentName(*response) : TString{};
    StateMachine_.Process(NMegamindEvents::TUsefulMegamindResponse(req.Start, finish, intentName));
    if (response) {
        // prepare the Apply request if needed
        const auto& proto = response->GetProtoResponse();
        for (const auto& directive : proto.GetResponse().GetDirectives()) {
            if (directive.GetType() == UNIPROXY_ACTION_DIRECTIVE_TYPE && directive.GetName() == DEFER_APPLY_DIRECTIVE_NAME && directive.HasPayload()) {
                HaveApply = true;
                {
                    TString logText{"Got DEFERRED_APPLY"};
                    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(logText);
                }
                NJson::TJsonValue payload = NAlice::JsonFromProto(directive.GetPayload());

                // prepare an APPLY request with modified session
                NAlice::TSpeechKitRequestProto request(req.Request);
                request.SetSession(payload["session"].GetString());  // will be empty if "session" missing or wrong type
                NAliceProtocol::TMegamindRequestTag requestTag;
                requestTag.SetTag(req.SetraceLabel);
                with_lock(MutexWriteToContext) {
                    AhContext().AddProtobufItem(request, ITEM_TYPE_MEGAMIND_APPLY_REQUEST);
                    AhContext().AddProtobufItem(requestTag, ITEM_TYPE_MEGAMIND_APPLY_REQUEST_TAG);
                }
                {
                    Censore(request);
                    TString logText{request.ShortUtf8DebugString()};
                    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMegamindApplyRequest>();
                }
            }
        }

        TryAddContextSaveRequest(*response);

        // prepare request for saving megamind session
        if (!HaveApply) {
            TryAddSessionSaveRequest(*response);
            auto mmResponse = PatchMegamindResponse(RequestCtx.GetRef(), *response, req);
            try {
                ProcessUsedMegamindResponse(*response);
                AddTtsRequestIfNeed(*response);
            } catch (const TInvalidMegamindDirective&) {
                LogContext.LogEventErrorCombo<NEvClass::TInvalidMegamindDirective>(CurrentExceptionMessage());
                OnError(CurrentExceptionMessage(), "invalid_mm_directive");
                return;
            } catch (...) {
                OnError(CurrentExceptionMessage(), "process_mm_resp");
                return;
            }
            {
                TString logText{mmResponse.ShortUtf8DebugString()};
                LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMegamindResponse>(logText);
            }
            StateMachine_.Process(NMegamindEvents::TSuccess());
            with_lock(MutexWriteToContext) {
                AhContext().AddProtobufItem(mmResponse, ITEM_TYPE_MEGAMIND_RESPONSE);
            }
        } else {
            StateMachine_.Process(NMegamindEvents::TSuccess(/*hasApply=*/true));
        }
        // Flush inside AddMegamindRunReady
        AddMegamindRunReady();
    } else {
        if (errorCode) {
            OnError(error.Defined() ? error.GetRef() : TString{}, errorCode);
        } else {
            OnInternalError("TMegamindRunServant::OnHasFinalResult request not has both - response & error", "internal_error34");
        }
    }
}

void TMegamindRunServant::OnCancelRequest() {
    StateMachine_.Process(NMegamindEvents::TCancelRequest());
}

void TMegamindRunServant::SendLegacySessionLogPartial(const TString& status) {
    NJson::TJsonValue val;
    val["status"] = status;
    SendSessionLogDirective(std::move(val), "VinsPartial");
}

// ------------------------------------------------------- TMegamindApplyServant

ERequestPhase TMegamindApplyServant::GetRequestPhase() const {
    return ERequestPhase::APPLY;
}

void TMegamindApplyServant::OnCreate() {
    LogContext.LogEvent(NEvClass::MegamindApplyAdapter());
    StateMachine_.Process(NSM::TStartEv());
}

void TMegamindApplyServant::OnContextsLoaded() {
    StateMachine_.Process(NMegamindEvents::TContextsLoaded(RequestCtx->GetHeader().GetFullName()));
}

bool TMegamindApplyServant::ProcessNextInput() {
    if (!ApplyRequest && AhContext().HasProtobufItem(ITEM_TYPE_MEGAMIND_APPLY_REQUEST)) {
        ApplyRequest = AhContext().GetOnlyProtobufItem<NAlice::TSpeechKitRequestProto>(ITEM_TYPE_MEGAMIND_APPLY_REQUEST);
        {
            NAlice::TSpeechKitRequestProto req{*ApplyRequest};
            Censore(req);
            TString logText{req.ShortUtf8DebugString()};
            LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostMegamindApplyRequest>(logText);
        }
        StateMachine_.Process(NMegamindEvents::TReceiveApplyRequest());
    }

    if (CanSendRequest() && ApplyRequest) {
        auto req = MakeIntrusive<TSubrequest>();
        req->Final = true;
        req->Request = *ApplyRequest;

        if (AhContext().HasProtobufItem(ITEM_TYPE_MEGAMIND_APPLY_REQUEST_TAG)) {
            const auto requestTag = AhContext().GetOnlyProtobufItem<NAliceProtocol::TMegamindRequestTag>(ITEM_TYPE_MEGAMIND_APPLY_REQUEST_TAG);
            req->SetraceLabel = requestTag.GetTag();
        } else {
            SetSetraceLabel(req);
        }
        SendRequest(req);
    }
    return true;
}

void TMegamindApplyServant::OnHasFinalResult(TSubrequest& req) {
    // get response|error from mutex lock
    TMaybe<TString> error;
    TString errorCode;
    TInstant finish;
    THolder<NAliceProtocol::TMegamindResponse> response;
    with_lock(req.Mutex) {
        Y_ASSERT(req.Final);
        req.Error.Swap(error);
        errorCode = req.ErrorCode;
        finish = req.Finish;
        req.Response.Swap(response);
    }
    StateMachine_.Process(NMegamindEvents::TUsefulMegamindResponse(req.Start, finish));
    if (response) {
        TryAddContextSaveRequest(*response);
        TryAddSessionSaveRequest(*response);
        try {
            ProcessUsedMegamindResponse(*response);
            AddTtsRequestIfNeed(*response);
        } catch (const TInvalidMegamindDirective&) {
            LogContext.LogEventErrorCombo<NEvClass::TInvalidMegamindDirective>(CurrentExceptionMessage());
            OnError(CurrentExceptionMessage(), "invalid_mm_directive");
            return;
        } catch (...) {
            OnError(CurrentExceptionMessage(), "exception_on_process_response");
            return;
        }
        auto mmResponse = PatchMegamindResponse(RequestCtx.GetRef(), *response, req);
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMegamindResponse>(mmResponse.ShortUtf8DebugString());
        StateMachine_.Process(NMegamindEvents::TSuccess());
        with_lock(MutexWriteToContext) {
            AhContext().AddProtobufItem(mmResponse, ITEM_TYPE_MEGAMIND_RESPONSE);
            FlushAndFinish<NEvClass::FlushAppHostContextAndFinishAfterFinalMegamindApplyResponse>();
        }
    } else {
        if (errorCode) {
            OnError(error.Defined() ? error.GetRef() : TString{}, errorCode);
        } else {
            OnInternalError("TMegamindApplyServant::OnHasFinalResult request not has both - response & error", "internal_error35");
        }
    }
}
