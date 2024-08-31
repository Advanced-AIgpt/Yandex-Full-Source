#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/context_load/cache.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/common/datasync.h>
#include <alice/cuttlefish/library/cuttlefish/synchronize_state/utils.h>

#include <alice/cuttlefish/library/aws/apphost_http_request_signer.h>
#include <alice/cuttlefish/library/aws/constants.h>

#include <alice/cuttlefish/library/protos/audio_separator.pb.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/context_save.pb.h>

#include <alice/library/cachalot_cache/cachalot_cache.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/push.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <alice/memento/proto/api.pb.h>

#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/schedule_action.pb.h>
#include <alice/protos/api/notificator/api.pb.h>

#include <alice/uniproxy/library/protos/notificator.pb.h>

#include <voicetech/library/messages/json_utils.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/lib/proto_answers/tvm_user_ticket.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/digest/city.h>

namespace NMemento = ru::yandex::alice::memento::proto;

using namespace NAlice::NAppHostServices;
using TCachalotResponse = NCachalotProtocol::TResponse;

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

constexpr TStringBuf UPDATE_DATASYNC_DIRECTIVE = "update_datasync";
constexpr TStringBuf UPDATE_MEMENTO_DIRECTIVE = "update_memento";
constexpr TStringBuf UPDATE_NOTIFICATION_SUBSCRIPTION_DIRECTIVE = "update_notification_subscription";
constexpr TStringBuf MARK_NOTIFICATION_AS_READ_DIRECTIVE = "mark_notification_as_read";
constexpr TStringBuf SEND_SUP_PUSH_DIRECTIVE = "push_message";
constexpr TStringBuf PERSONAL_CARDS_DIRECTIVE = "personal_cards";
constexpr TStringBuf SEND_PUSH_DIRECTIVE = "send_push_directive";
constexpr TStringBuf DELETE_PUSHES_DIRECTIVE = "delete_pushes_directive";
constexpr TStringBuf PUSH_TYPED_SEMANTIC_FRAME_DIRECTIVE = "push_typed_semantic_frame";
constexpr TStringBuf ADD_SCHEDULE_ACTION_DIRECTIVE = "add_schedule_action";
constexpr TStringBuf SAVE_USER_AUDIO_DIRECTIVE = "save_user_audio";
constexpr TStringBuf PATCH_ASR_OPTIONS_FOR_NEXT_REQUEST_DIRECTIVE = "patch_asr_options_for_next_request";

constexpr TStringBuf ASR_OPTIONS_STORAGE_TAG = "AsrOptions";
constexpr TStringBuf CACHALOT_SAVE_ASR_OPTIONS_PATCH_SOURCE = "CACHALOT_SAVE_ASR_OPTIONS_PATCH";

// Sensor's reslated consts.
constexpr TStringBuf SENSOR_RATE_BACKEND_CSD = "context_save_directive";
constexpr TStringBuf SENSOR_RATE_CODE_ERROR = "error";
constexpr TStringBuf SENSOR_RATE_CODE_NOANS = "noans";
constexpr TStringBuf SENSOR_RATE_CODE_OK = "ok";
constexpr TStringBuf SENSOR_RATE_DIRECTIVE = "directive";
constexpr TStringBuf SENSOR_RATE_DIRECTIVE_RESPONSE = "directive_response";
constexpr TStringBuf SENSOR_RATE_RESPONSE = "response";

class TContextSaveProcessor {
public:
    TContextSaveProcessor(NAppHost::IServiceContext& ctx, TLogContext logContext, TStringBuf source = "context_save");

    void Pre();
    void Post();
    void Fake();

private:
    void PreDatasync(const TVector<NJson::TJsonValue>& payloads);
    void PreMemento(const TVector<NJson::TJsonValue>& payloads);
    void PreNotificatorSubcription(const NJson::TJsonValue& payload);
    void PreNotificatorMarkAsRead(const NJson::TJsonValue& payload);
    void PrePersonalCards(const NJson::TJsonValue& payload);
    void PreNotificatorSendSupPush(const NJson::TJsonValue& payload);
    void PreSendPushDirective(const NJson::TJsonValue& payload);
    void PreDeletePushesDirective(const NJson::TJsonValue& payload);
    void PrePushTypedSemanticFrameDirective(const NJson::TJsonValue& payload);
    void PreAddScheduleActionDirective(const NJson::TJsonValue& payload);
    void PreSaveUserAudioDirective(const NJson::TJsonValue& payload);
    void PrePatchAsrOptionsForNextRequestDirective(const google::protobuf::Struct& payload);
    void PreContextSaveDirective(const NSpeechKit::TProtobufUniproxyDirective::TContextSaveDirective& directive);
    void PreInvalidateDatasyncCache();

private:
    NAppHost::IServiceContext& Ctx;
    TLogContext LogContext;
    TSourceMetrics Metrics;

    const NAliceProtocol::TSessionContext SessionCtx;
    const NAliceProtocol::TRequestContext RequestCtx;
    const TString UserTicket;
    const TString BlackboxUid;
    const TMaybe<NAliceProtocol::TFullIncomingAudio> FullIncomingAudio;
};


const NAliceProtocol::TSessionContext TryLoadSessionContext(const NAppHost::IServiceContext& ctx) {
    NAliceProtocol::TSessionContext sessionCtx;
    if (ctx.HasProtobufItem(ITEM_TYPE_SESSION_CONTEXT)) {
        sessionCtx = ctx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT);
    }
    return sessionCtx;
}

const NAliceProtocol::TRequestContext TryLoadRequestContext(const NAppHost::IServiceContext& ctx) {
    NAliceProtocol::TRequestContext requestCtx;
    if (ctx.HasProtobufItem(ITEM_TYPE_REQUEST_CONTEXT)) {
        requestCtx = ctx.GetOnlyProtobufItem<NAliceProtocol::TRequestContext>(ITEM_TYPE_REQUEST_CONTEXT);
    }
    return requestCtx;
}

const TString TryLoadUserTicket(const NAppHost::IServiceContext& ctx) {
    if (ctx.HasProtobufItem(ITEM_TYPE_TVM_USER_TICKET)) {
        return ctx.GetOnlyProtobufItem<NAppHostTvmUserTicket::TTvmUserTicket>(
            ITEM_TYPE_TVM_USER_TICKET
        ).GetUserTicket();
    }
    return TString();
}

TString TryLoadBlackboxUid(const NAppHost::IServiceContext& ctx) {
    if (ctx.HasProtobufItem(ITEM_TYPE_BLACKBOX_UID)) {
        return ctx.GetOnlyProtobufItem<NAliceProtocol::TContextLoadBlackboxUid>(
            ITEM_TYPE_BLACKBOX_UID
        ).GetUid();
    }
    return TString();
}

TMaybe<NAliceProtocol::TFullIncomingAudio> TryLoadFullIncomingAudio(const NAppHost::IServiceContext& ctx) {
    if (ctx.HasProtobufItem(ITEM_TYPE_FULL_INCOMING_AUDIO)) {
        const auto fullIncomingAudio = ctx.GetOnlyProtobufItem<NAliceProtocol::TFullIncomingAudio>(ITEM_TYPE_FULL_INCOMING_AUDIO);
        return fullIncomingAudio;
    }
    return Nothing();
}

TContextSaveProcessor::TContextSaveProcessor(NAppHost::IServiceContext& ctx, TLogContext logContext, TStringBuf source)
    : Ctx(ctx)
    , LogContext(logContext)
    , Metrics(ctx, source)
    , SessionCtx(TryLoadSessionContext(ctx))
    , RequestCtx(TryLoadRequestContext(ctx))
    , UserTicket(TryLoadUserTicket(ctx))
    , BlackboxUid(TryLoadBlackboxUid(ctx))
    , FullIncomingAudio(TryLoadFullIncomingAudio(ctx))
{}

void TContextSaveProcessor::Pre() {
    if (!Ctx.HasProtobufItem(ITEM_TYPE_CONTEXT_SAVE_REQUEST)) {
        return;
    }

    const auto request = Ctx.GetOnlyProtobufItem<NAliceProtocol::TContextSaveRequest>(ITEM_TYPE_CONTEXT_SAVE_REQUEST);
    TVector<NJson::TJsonValue> datasyncPayloads;
    TVector<NJson::TJsonValue> mementoPayloads;

    // There are normally ~1-3 directives per request
    // so TSet is better than THashSet (use NYT::TCompactSet?)
    TSet<TString> requestCreatedForDirectives;
    TSet<TString> failedDirectives;

    // Data for Post node.
    NAliceProtocol::TContextSavePreRequestsInfo contextSavePreRequestsInfo;

    auto processDirectiveSafe = [this, &failedDirectives](const std::function<void()>& directiveProcessor, TStringBuf directiveName) {
        try {
            directiveProcessor();
        } catch (...) {
            Metrics.PushRate("failed_directive", directiveName);
            LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Failed to process directive '" << directiveName << "': " << CurrentExceptionMessage());
            failedDirectives.insert(TString(directiveName));
        }
    };

    for (const auto& directive : request.GetContextSaveDirectives()) {
        processDirectiveSafe(
            [this, &directive]() {
                PreContextSaveDirective(directive);
            },
            SENSOR_RATE_BACKEND_CSD
        );
    }

    for (const auto& directive : request.GetDirectives()) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Working with directive " << directive.GetName());

        if (!directive.HasPayload()) {
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Directive " << directive.GetName() << " has no payload, skip");
            continue;
        }
        const google::protobuf::Struct& payloadStruct = directive.GetPayload();
        NJson::TJsonValue payload = NAlice::JsonFromProto(payloadStruct);
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
            TStringBuilder()
                << "Directive " << directive.GetName()
                << " has payload " << NJson::WriteJson(payload, /* formatOutput = */ false)
        );

        const TString& name = directive.GetName();
        // Datasync and memento are special (we allow multiple directives of this type)
        if (name == UPDATE_DATASYNC_DIRECTIVE) {
            datasyncPayloads.emplace_back(std::move(payload));
            continue;
        } else if (name == UPDATE_MEMENTO_DIRECTIVE) {
            mementoPayloads.emplace_back(std::move(payload));
            continue;
        }

        if (requestCreatedForDirectives.contains(name)) {
            Metrics.PushRate("duplicate_directive", "error");
            LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Two or more directives with name '" << name << "'");
            failedDirectives.insert(name);
            continue;
        }

        auto commonDirectiveProcessor = [this, &name, &payload, &payloadStruct, &requestCreatedForDirectives, &failedDirectives]() {
            if (name == UPDATE_NOTIFICATION_SUBSCRIPTION_DIRECTIVE) {
                PreNotificatorSubcription(std::move(payload));
            } else if (name == MARK_NOTIFICATION_AS_READ_DIRECTIVE) {
                PreNotificatorMarkAsRead(std::move(payload));
            } else if (name == SEND_SUP_PUSH_DIRECTIVE) {
                PreNotificatorSendSupPush(std::move(payload));
            } else if (name == PERSONAL_CARDS_DIRECTIVE) {
                PrePersonalCards(std::move(payload));
            } else if (name == SEND_PUSH_DIRECTIVE) {
                PreSendPushDirective(std::move(payload));
            } else if (name == DELETE_PUSHES_DIRECTIVE) {
                PreDeletePushesDirective(std::move(payload));
            } else if (name == PUSH_TYPED_SEMANTIC_FRAME_DIRECTIVE) {
                PrePushTypedSemanticFrameDirective(std::move(payload));
            } else if (name == ADD_SCHEDULE_ACTION_DIRECTIVE) {
                PreAddScheduleActionDirective(std::move(payload));
            } else if (name == SAVE_USER_AUDIO_DIRECTIVE) {
                PreSaveUserAudioDirective(std::move(payload));
            } else if (name == PATCH_ASR_OPTIONS_FOR_NEXT_REQUEST_DIRECTIVE) {
                PrePatchAsrOptionsForNextRequestDirective(payloadStruct);
            } else {
                Metrics.PushRate("unknown_directive", "error");
                LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Skip unknown directive '" << name << "'");
                failedDirectives.insert(name);
                // Do not add name to requestCreatedForDirectives
                // Do not throw here to report more proper fail reason
                return;
            }

            requestCreatedForDirectives.insert(name);
        };

        processDirectiveSafe(commonDirectiveProcessor, name);
    }

    if (!datasyncPayloads.empty()) {
        processDirectiveSafe(
            [this, &requestCreatedForDirectives, &datasyncPayloads]() {
                PreDatasync(datasyncPayloads);
                requestCreatedForDirectives.insert(TString(UPDATE_DATASYNC_DIRECTIVE));
            },
            UPDATE_DATASYNC_DIRECTIVE
        );

        // Invalidate cache if it is enabled. The updated Datasync data will be issued and cached on next request
        PreInvalidateDatasyncCache();
    }
    if (!mementoPayloads.empty()) {
        processDirectiveSafe(
            [this, &requestCreatedForDirectives, &mementoPayloads]() {
                PreMemento(std::move(mementoPayloads));
                requestCreatedForDirectives.insert(TString(UPDATE_MEMENTO_DIRECTIVE));
            },
            UPDATE_MEMENTO_DIRECTIVE
        );
    }

    for (const auto& directiveName : requestCreatedForDirectives) {
        *contextSavePreRequestsInfo.AddRequestCreatedForDirectives() = directiveName;
    }
    for (const auto& directiveName : failedDirectives) {
        *contextSavePreRequestsInfo.AddFailedDirectives() = directiveName;
    }
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostContextSavePreRequestsInfo>(contextSavePreRequestsInfo.ShortUtf8DebugString());
    Ctx.AddProtobufItem(contextSavePreRequestsInfo, ITEM_TYPE_CONTEXT_SAVE_PRE_REQUESTS_INFO);
}

void TContextSaveProcessor::Post() {
    NAliceProtocol::TContextSaveResponse response;
    TSet<TString> failedDirectives;

    if (Ctx.HasProtobufItem(ITEM_TYPE_CONTEXT_SAVE_PRE_REQUESTS_INFO)) {
        auto contextSavePreRequestsInfo = Ctx.GetOnlyProtobufItem<NAliceProtocol::TContextSavePreRequestsInfo>(ITEM_TYPE_CONTEXT_SAVE_PRE_REQUESTS_INFO);
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostContextSavePreRequestsInfo>(contextSavePreRequestsInfo.ShortUtf8DebugString());

        for (const auto& failedDirectiveName : contextSavePreRequestsInfo.GetFailedDirectives()) {
            failedDirectives.insert(failedDirectiveName);
        }

        for (const auto& check : contextSavePreRequestsInfo.GetCheckResponseForDirectiveRequest()) {
            const auto directiveItemName = TString::Join(check.GetDirectiveName(), '.', check.GetOutputType());

            if (!Ctx.HasProtobufItem(check.GetOutputType())) {

                Metrics.PushRate(SENSOR_RATE_DIRECTIVE_RESPONSE, SENSOR_RATE_CODE_NOANS, directiveItemName);
                LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(
                    TStringBuilder() << "Unable to find response item '" << check.GetOutputType() << "' for directive: " << check.GetDirectiveName()
                );

                failedDirectives.insert(directiveItemName);
            } else {
                Metrics.PushRate(SENSOR_RATE_DIRECTIVE_RESPONSE, SENSOR_RATE_CODE_OK, directiveItemName);
            }
        }
    }

    // TODO(VOICESERV-4184) use TContextSavePreRequestInfo::RequestCreatedForDirectives to handle noans

    // Check http responses for errors
    static const TVector<std::tuple<TStringBuf, TStringBuf, TStringBuf, TStringBuf>> httpInfo = {
        {
            ITEM_TYPE_DATASYNC_HTTP_REQUEST,
            ITEM_TYPE_DATASYNC_HTTP_RESPONSE,
            UPDATE_DATASYNC_DIRECTIVE,
            "datasync",
        }, {
            ITEM_TYPE_MEMENTO_HTTP_REQUEST,
            ITEM_TYPE_MEMENTO_HTTP_RESPONSE,
            UPDATE_MEMENTO_DIRECTIVE,
            "memento",
        }, {
            ITEM_TYPE_NOTIFICATOR_SUBSCRIPTION_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_SUBSCRIPTION_HTTP_RESPONSE,
            UPDATE_NOTIFICATION_SUBSCRIPTION_DIRECTIVE,
            "notificator-subscribe",
        }, {
            ITEM_TYPE_NOTIFICATOR_MARK_AS_READ_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_MARK_AS_READ_HTTP_RESPONSE,
            MARK_NOTIFICATION_AS_READ_DIRECTIVE,
            "notificator-read",
        }, {
            ITEM_TYPE_NOTIFICATOR_SEND_SUP_PUSH_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_SEND_SUP_PUSH_HTTP_RESPONSE,
            SEND_SUP_PUSH_DIRECTIVE,
            "notificator-sup",
        }, {
            ITEM_TYPE_PERSONAL_CARDS_DISMISS_HTTP_REQUEST,
            ITEM_TYPE_PERSONAL_CARDS_DISMISS_HTTP_RESPONSE,
            PERSONAL_CARDS_DIRECTIVE,
            "personal-cards-dismiss",
        }, {
            ITEM_TYPE_PERSONAL_CARDS_ADD_HTTP_REQUEST,
            ITEM_TYPE_PERSONAL_CARDS_ADD_HTTP_RESPONSE,
            PERSONAL_CARDS_DIRECTIVE,
            "personal-cards",
        }, {
            ITEM_TYPE_NOTIFICATOR_SUP_CARD_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_SUP_CARD_HTTP_RESPONSE,
            SEND_PUSH_DIRECTIVE,
            "notificator-sup-card",
        }, {
            ITEM_TYPE_NOTIFICATOR_DELETE_PERSONAL_CARDS_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_DELETE_PERSONAL_CARDS_HTTP_RESPONSE,
            DELETE_PUSHES_DIRECTIVE,
            "notificator-delete-cards",
        }, {
            ITEM_TYPE_NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_HTTP_REQUEST,
            ITEM_TYPE_NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_HTTP_RESPONSE,
            PUSH_TYPED_SEMANTIC_FRAME_DIRECTIVE,
            "notificator-push-typed-semantic-frame",
        }, {
            ITEM_TYPE_MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_HTTP_REQUEST,
            ITEM_TYPE_MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_HTTP_RESPONSE,
            ADD_SCHEDULE_ACTION_DIRECTIVE,
            "matrix-scheduler-add-schedule-action",
        }, {
            ITEM_TYPE_S3_SAVE_USER_AUDIO_HTTP_REQUEST,
            ITEM_TYPE_S3_SAVE_USER_AUDIO_HTTP_RESPONSE,
            SAVE_USER_AUDIO_DIRECTIVE,
            "s3-save-user-audio",
        }
    };

    for (const auto& [requestType, responseType, directiveName, backendName] : httpInfo) {
        if (Ctx.HasProtobufItem(responseType)) {
            auto httpResp = Ctx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(responseType);
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << responseType << " HTTP response: " << httpResp);

            bool isHttpError = httpResp.GetStatusCode() / 100 != 2;
            if (isHttpError) {
                failedDirectives.insert(TString(directiveName));
                Metrics.PushRate("response", "error", backendName);
            } else {
                Metrics.PushRate("response", "ok", backendName);
            }
        } else if (Ctx.HasProtobufItem(requestType)) {
            // IMP: `NOT` condition is not required, we are checking request, not response
            // the request was sent, but no response received
            failedDirectives.insert(TString(directiveName));
            Metrics.PushRate(SENSOR_RATE_RESPONSE, SENSOR_RATE_CODE_NOANS, backendName);
        }
    }

    // Check cachalot responses
    static const TVector<std::tuple<TStringBuf, TStringBuf, TStringBuf, TStringBuf>> cachalotInfo = {
        {
            ITEM_TYPE_MEGAMIND_SESSION_REQUEST,
            ITEM_TYPE_MEGAMIND_SESSION_RESPONSE,
            "cachalot_mm_session",
            "cachalot-mm-session",
        }, {
            ITEM_TYPE_CACHALOT_SAVE_ASR_OPTIONS_PATCH_REQUEST,
            ITEM_TYPE_CACHALOT_SAVE_ASR_OPTIONS_PATCH_RESPONSE,
            PATCH_ASR_OPTIONS_FOR_NEXT_REQUEST_DIRECTIVE,
            "cachalot-save-asr-options-patch",
        }
    };

    for (const auto& [requestType, responseType, directiveName, backendName] : cachalotInfo) {
        if (Ctx.HasProtobufItem(responseType)) {
            const auto cachalotResponse = Ctx.GetOnlyProtobufItem<TCachalotResponse>(responseType);
            const auto status = cachalotResponse.GetStatus();

            // `status` shall be always "CREATED", but we don't fail "OK"
            if (status != NCachalotProtocol::CREATED && status != NCachalotProtocol::OK) {
                Metrics.PushRate(SENSOR_RATE_RESPONSE, SENSOR_RATE_CODE_ERROR, backendName);
                if (requestType == ITEM_TYPE_MEGAMIND_SESSION_REQUEST) {
                    // MM session request is special
                    response.SetFailedMegamindSession(true);
                } else {
                    failedDirectives.insert(TString(directiveName));
                }
                LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(
                    TStringBuilder()
                        << directiveName << " directive failed: (" << static_cast<int>(status) << ") "
                        << cachalotResponse.GetStatusMessage()
                );
            } else {
                Metrics.PushRate(SENSOR_RATE_RESPONSE, SENSOR_RATE_CODE_OK, backendName);
                LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
                    TStringBuilder()
                        << directiveName << " directive successful: (" << static_cast<int>(status) << ") "
                        << cachalotResponse.GetStatusMessage()
                );
            }
        } else if (Ctx.HasProtobufItem(requestType)) {
            if (requestType == ITEM_TYPE_MEGAMIND_SESSION_REQUEST) {
                // MM session request is special
                response.SetFailedMegamindSession(true);
            } else {
                failedDirectives.insert(TString(directiveName));
            }
            Metrics.PushRate(SENSOR_RATE_RESPONSE, SENSOR_RATE_CODE_NOANS, backendName);
        }
    }

    for (const auto& failedDirectiveName : failedDirectives) {
        *response.AddFailedDirectives() = failedDirectiveName;
    }

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostContextSaveResponse>(response.ShortUtf8DebugString());
    Ctx.AddProtobufItem(response, ITEM_TYPE_CONTEXT_SAVE_RESPONSE);
}

void TContextSaveProcessor::Fake() {
    NAliceProtocol::TContextSaveResponse response;
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostContextSaveResponse>(response.ShortUtf8DebugString());
    Ctx.AddProtobufItem(response, ITEM_TYPE_CONTEXT_SAVE_RESPONSE);
}

void TContextSaveProcessor::PreContextSaveDirective(const NSpeechKit::TProtobufUniproxyDirective::TContextSaveDirective& csd) {
    const auto& directiveId = csd.GetDirectiveId();
    if (directiveId.Empty()) {
        Metrics.PushRate(SENSOR_RATE_DIRECTIVE, "empty_directive_id", SENSOR_RATE_BACKEND_CSD);
        ythrow yexception() << "CSD failed: DirectiveId is empty";
    }

    const auto sensorBackendName = TString::Join(SENSOR_RATE_BACKEND_CSD, '.', directiveId);

    const google::protobuf::Any& any = csd.GetPayload();
    if (any.type_url().empty()) {
        Metrics.PushRate(SENSOR_RATE_DIRECTIVE, "empty_body", sensorBackendName);
        ythrow yexception() << "CSD failed: payload protobuf.any is not initialized for " << sensorBackendName;
    }

    Ctx.AddRawItem(NAppHost::NService::TRawItem{
                       .Data = any.value(),
                       .Format = NAppHost::EItemType::Protobuf,
                   },
                   directiveId);

    Metrics.PushRate(SENSOR_RATE_DIRECTIVE, SENSOR_RATE_CODE_OK, sensorBackendName);

    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
        TStringBuilder() << "Added item '" << directiveId
                         << "' from CSD with proto type: " << any.type_url());
}

void TContextSaveProcessor::PreDatasync(const TVector<NJson::TJsonValue>& payloads) {
    Metrics.PushRate("directive", "ok", "datasync");

    NAppHostHttp::THttpRequest req = TDatasyncClient::SaveVinsContextsRequest(payloads);

    if (!UserTicket.empty() && !BlackboxUid.empty()) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Use UserTicket in header \"X-Ya-User-Ticket\"");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Use Uid in header \"X-Uid\"");
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncHttpRequest>(req.ShortUtf8DebugString());
        TDatasyncClient::AddUserTicketHeader(req, UserTicket);
        TDatasyncClient::AddUidHeader(req, BlackboxUid);
    } else {
        const auto& info = SessionCtx.GetUserInfo();

        TStringBuf uuid;
        if (info.HasVinsApplicationUuid()) {
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Use VinsApplicationUuid in header \"X-Uid\"");
            uuid = info.GetVinsApplicationUuid();
        } else if (info.HasUuid()) {
            uuid = info.GetUuid();
        } else {
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("We will not send datasync request, failed to create id header!");
            return;
        }
        TDatasyncClient::AddDeviceIdHeader(req, uuid);
        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncHttpRequest>(req.ShortUtf8DebugString());
    }

    Ctx.AddProtobufItem(req, ITEM_TYPE_DATASYNC_HTTP_REQUEST);
}

void TContextSaveProcessor::PreMemento(const TVector<NJson::TJsonValue>& payloads) {
    if (!NExpFlags::ConductingExperiment(RequestCtx, "use_memento")) {
        Metrics.PushRate("directive", "noexp", "memento");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreMemento: don't save because there is no experiment flag \"use_memento\"");
        return;
    }
    if (UserTicket.Empty()) {
        Metrics.PushRate("directive", "noticket", "memento");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreMemento: don't save because user ticket is empty");
        return;
    }

    // read payload
    NMemento::TReqChangeUserObjects mementoReq;
    bool requestIsValid = false;

    for (const NJson::TJsonValue& payload : payloads) {
        if (!payload.Has("user_objects")) {
            Metrics.PushRate("directive", "nodata", "memento");
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
                "PreMemento: don't save because payload has no \"user_objects\""
            );
            continue;
        }

        const TString& userObjects = payload["user_objects"].GetString();
        if (userObjects.empty()) {
            Metrics.PushRate("directive", "nodata", "memento");
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
                "PreMemento: don't save because payload \"user_objects\" is empty"
            );
            continue;
        }

        // using separated proto here in order to protect resulting proto from corrupted data.
        NMemento::TReqChangeUserObjects mementoReqPart;
        if (!mementoReqPart.ParseFromString(Base64Decode(userObjects))) {
            Metrics.PushRate("directive", "nodata", "memento");
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreMemento: unable to parse payload");
        } else {
            mementoReq.MergeFrom(mementoReqPart);
            requestIsValid = true;
        }
    }

    if (!requestIsValid) {
        Metrics.PushRate("directive", "nodata", "memento");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreMemento: all payloads are invalid");
        return;
    }

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetPath("/memento/update_objects");
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetContent(mementoReq.SerializeAsString());
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMementoHttpRequest>(req.ShortUtf8DebugString(), mementoReq.ShortUtf8DebugString());
    AddHeader(req, "X-Ya-User-Ticket", UserTicket);
    Ctx.AddProtobufItem(req, ITEM_TYPE_MEMENTO_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "memento");
}

void TContextSaveProcessor::PreNotificatorSubcription(const NJson::TJsonValue& payload) {
    // read payload
    const NJson::TJsonValue* subscriptionId = payload.GetValueByPath("subscription_id");
    const NJson::TJsonValue* unsubscribe = payload.GetValueByPath("unsubscribe");
    if (!subscriptionId || !unsubscribe) {
        Metrics.PushRate("directive", "nodata", "notificator-subscribe");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreNotificatorSubcription: don't save because payload has no \"subscription_id\" or \"unsubscribe\" field");
        return;
    }

    int subId = subscriptionId->GetInteger();
    bool needSubscribe = !unsubscribe->GetBoolean();
    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "PreNotificatorSubcription: subscriptionId = " << subId << ", needSubscribe = " << needSubscribe);

    // construct protobuf
    ::NNotificator::TManageSubscription msg;
    msg.SetSubscriptionId(subId);
    msg.SetMethod(needSubscribe ? ::NNotificator::TManageSubscription::ESubscribe : ::NNotificator::TManageSubscription::EUnsubscribe);
    msg.SetPuid(SessionCtx.GetUserInfo().GetPuid());
    if (SessionCtx.HasAppId()) {
        msg.SetAppId(SessionCtx.GetAppId());
    }

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/subscriptions/manage");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNotificationSubscriptionHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_SUBSCRIPTION_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "notificator-subscribe");
}

void TContextSaveProcessor::PreNotificatorMarkAsRead(const NJson::TJsonValue& payload) {
    // read payload
    const NJson::TJsonValue* idsList = payload.GetValueByPath("notification_ids");
    const NJson::TJsonValue* id = payload.GetValueByPath("notification_id");
    if (!idsList && !id) {
        Metrics.PushRate("directive", "nodata", "notificator-read");
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("PreNotificatorMarkAsRead: don't save because payload has no \"notification_ids\" and \"notification_id\" fields");
        return;
    }

    // construct protobuf
    ::NNotificator::TNotificationChangeStatus msg;
    if (idsList) {
        for (const auto& elem : idsList->GetArray()) {
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "PreNotificatorMarkAsRead: add notification id" << elem.GetString() << " (from notification_ids)");
            msg.AddNotificationIds(elem.GetString());
        }
    }
    if (id) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "PreNotificatorMarkAsRead: add notification id" << id->GetString() << " (from notification_id)");
        msg.AddNotificationIds(id->GetString());
    }

    msg.SetStatus(::NNotificator::ENotificationRead);
    msg.SetPuid(SessionCtx.GetUserInfo().GetPuid());
    if (SessionCtx.HasAppId()) {
        msg.SetAppId(SessionCtx.GetAppId());
    }

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/notifications/change_status");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNotificatorMarkAsrReadHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_MARK_AS_READ_HTTP_REQUEST);
    Metrics.PushRate("directive", "ok", "notificator-read");
}

void TContextSaveProcessor::PreNotificatorSendSupPush(const NJson::TJsonValue& payload) {
    if (!SessionCtx.HasUserInfo() || !SessionCtx.GetUserInfo().HasPuid()) {
        Metrics.PushRate("directive", "nodata", "notificator-sup");
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>("[send_sup_push] puid is not set");
        return;
    }

    // read payload & construct protobuf
    NAlice::NScenarios::TPushMessageDirective pushMsg;
    NProtobufJson::Json2Proto(payload, pushMsg, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));

    ::NNotificator::TSupMessage msg;
    msg.MutablePushMsg()->Swap(&pushMsg);
    msg.SetPuid(SessionCtx.GetUserInfo().GetPuid());
    if (SessionCtx.HasAppId()) {
        msg.SetAppId(SessionCtx.GetAppId());
    }
    if (const auto testId = NExpFlags::GetExperimentValue(RequestCtx, "notificator_sup_test_id")) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "PreNotificatorSendSupPush: use testId " << *testId << " from experiment \"notificator_sup_test_id\"");
        msg.SetTestId(*testId);
    }

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/delivery/sup");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNotificatorSendSupPushHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_SEND_SUP_PUSH_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "notificator-sup");
}


void TContextSaveProcessor::PrePersonalCards(const NJson::TJsonValue& payload) {
    static const TVector<TStringBuf> cardTypes = {"yandex.station_film"};

    // read payload
    const auto& card = payload["card"];

    bool cardTypePresent = AnyOf(cardTypes, [&card](const TStringBuf cardType) { return card.Has(cardType); });
    if (!cardTypePresent) {
        LogContext.LogEventInfoCombo<NEvClass::ErrorMessage>("None of the card types present");
        Metrics.PushRate("directive", "nodata", "personal-cards");
        return;
    }

    NJson::TJsonValue addCardData;
    EnsureNodeByPath(addCardData, "card", "card", "card_id") = std::move(card["card_id"]);
    EnsureNodeByPath(addCardData, "card", "card", "date_from") = std::move(card["date_from"]);
    EnsureNodeByPath(addCardData, "card", "card", "date_to") = std::move(card["date_to"]);
    EnsureNodeByPath(addCardData, "uid") = SessionCtx.GetUserInfo().GetPuid();

    for (const TStringBuf cardType : cardTypes) {
        if (card.Has(cardType)) {
            EnsureNodeByPath(addCardData, "card", "card", "type") = cardType;

            auto& cardTypePart = card[cardType];
            if (cardTypePart.IsMap()) {
                EnsureNodeByPath(addCardData, "card", "card", "data") = std::move(card[cardType]);
            }
        }
    }

    EnsureNodeByPath(addCardData, "card", "card", "data", "button_url") = std::move(card["button_url"]);
    EnsureNodeByPath(addCardData, "card", "card", "data", "text") = std::move(card["text"]);

    if (payload["remove_existing_cards"].GetBooleanSafe(/* defaultValue = */ false)) {
        NJson::TJsonValue dismissData;
        EnsureNodeByPath(dismissData, "auth", "uid") = SessionCtx.GetUserInfo().GetPuid();
        EnsureNodeByPath(dismissData, "card_id") = "*";

        // construct http request for dismiss
        NAppHostHttp::THttpRequest req;
        req.SetMethod(NAppHostHttp::THttpRequest::Post);
        req.SetPath("/dismiss");
        req.SetContent(NJson::WriteJson(dismissData, /* formatOutput = */ false));

        LogContext.LogEventInfoCombo<NEvClass::SendToAppHostPersonalCardsDismissHttpRequest>(req.ShortUtf8DebugString());
        Ctx.AddProtobufItem(req, ITEM_TYPE_PERSONAL_CARDS_DISMISS_HTTP_REQUEST);
        Metrics.PushRate("directive", "ok", "personal-cards-dismiss");
    }

    // construct http request for add
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/addPushCards");
    req.SetContent(NJson::WriteJson(addCardData, /* formatOutput = */ false));

    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostPersonalCardsAddHttpRequest>(req.ShortUtf8DebugString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_PERSONAL_CARDS_ADD_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "personal-cards");
}

void TContextSaveProcessor::PreSendPushDirective(const NJson::TJsonValue& payload) {
    if (!SessionCtx.HasUserInfo() || !SessionCtx.GetUserInfo().HasPuid()) {
        Metrics.PushRate("directive", "nodata", "notificator-sup-card");
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>("[send_push_directive] puid is not set");
        return;
    }

    // read payload & construct protobuf
    NAlice::NScenarios::TSendPushDirective directive;
    NProtobufJson::Json2Proto(payload, directive, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));

    NNotificator::TDeliverySupCardRequest msg;

    msg.SetPuid(SessionCtx.GetUserInfo().GetPuid());
    msg.MutableDirective()->Swap(&directive);
    if (SessionCtx.HasAppId()) {
        msg.SetAppId(SessionCtx.GetAppId());
    }
    if (const auto testId = NExpFlags::GetExperimentValue(RequestCtx, "notificator_sup_test_id")) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(
            TStringBuilder()
                << "PreNotificatorSendPushDirective: use testId " << *testId
                << " from experiment \"notificator_sup_test_id\""
        );
        msg.SetTestId(*testId);
    }

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/delivery/sup_card");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNotificatorSupCardHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());

    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_SUP_CARD_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "notificator-sup-card");
}

void TContextSaveProcessor::PreDeletePushesDirective(const NJson::TJsonValue& payload) {
    if (!SessionCtx.HasUserInfo() || !SessionCtx.GetUserInfo().HasPuid()) {
        Metrics.PushRate("directive", "nodata", "notificator-delete-cards");
        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>("[delete_pushes_directive] puid is not set");
        return;
    }

    // read payload & construct protobuf
    NAlice::NScenarios::TDeletePushesDirective directive;
    NProtobufJson::Json2Proto(payload, directive, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));

    NNotificator::TDeletePersonalCards msg;

    msg.SetPuid(SessionCtx.GetUserInfo().GetPuid());
    msg.MutableDirective()->Swap(&directive);

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/personal_cards/delete");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostDeletePersonalCardsHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());

    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_DELETE_PERSONAL_CARDS_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "notificator-delete-cards");
}

void TContextSaveProcessor::PrePushTypedSemanticFrameDirective(const NJson::TJsonValue& payload) {
    // read payload & construct protobuf
    const auto msg = NAlice::JsonToProto<NMatrix::NApi::TDelivery>(payload);

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/delivery/push");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostNotificatorPushTypedSemanticFrameHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_PUSH_TYPED_SEMANTIC_FRAME_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "notificator-push-typed-semantic-frame");
}

void TContextSaveProcessor::PreAddScheduleActionDirective(const NJson::TJsonValue& payload) {
    // read payload & construct protobuf
    const auto msg = NAlice::JsonToProto<NMatrix::NApi::TScheduleAction>(payload["schedule_action"]);

    // construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetPath("/schedule");
    AddHeader(req, "Content-Type", "application/protobuf");
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostMatrixSchedulerAddScheduleActionHttpRequest>(req.ShortUtf8DebugString(), msg.ShortUtf8DebugString());
    req.SetContent(msg.SerializeAsString());
    Ctx.AddProtobufItem(req, ITEM_TYPE_MATRIX_SCHEDULER_ADD_SCHEDULE_ACTION_HTTP_REQUEST);

    Metrics.PushRate("directive", "ok", "matrix-scheduler-add-schedule-action");
}

void TContextSaveProcessor::PreSaveUserAudioDirective(const NJson::TJsonValue& payload) {
    if (!FullIncomingAudio.Defined()) {
        Metrics.PushRate("directive", "nodata", "save-user-audio");
        ythrow yexception() << "No user audio in apphost context";
    }

    if (FullIncomingAudio->HasErrorMessage()) {
        Metrics.PushRate("directive", "baddata", "save-user-audio");
        ythrow yexception() << "Error in collecting full incoming audio: " << FullIncomingAudio->GetErrorMessage();
    }

    // read payload & construct protobuf
    NAlice::NScenarios::TSaveUserAudioDirective directive;
    NProtobufJson::Json2Proto(payload, directive, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));

    if (directive.GetStorageOptions().GetStorageCase() != NAlice::NScenarios::TSaveUserAudioDirective::TStorageOptions::kS3Storage) {
        Metrics.PushRate("directive", "invalid", "save-user-audio");
        ythrow yexception() << "Invalid storage (only s3 implemented for now)";
    }

    TStringBuf audioData;
    switch (directive.GetPartsToSave()) {
        case NAlice::NScenarios::TSaveUserAudioDirective::MainPartOnly: {
            audioData = FullIncomingAudio->GetMainPart();
            break;
        }
        case NAlice::NScenarios::TSaveUserAudioDirective::SpotterPartOnly: {
            audioData = FullIncomingAudio->GetSpotterPart();
            break;
        }
        case NAlice::NScenarios::TSaveUserAudioDirective::Unknown:
        // We don't want to use default (without default adding a new enum value will result in a compilation error)
        // So we need this to handle all cases
        case NAlice::NScenarios::TSaveUserAudioDirective_EPartsToSave_TSaveUserAudioDirective_EPartsToSave_INT_MIN_SENTINEL_DO_NOT_USE_:
        case NAlice::NScenarios::TSaveUserAudioDirective_EPartsToSave_TSaveUserAudioDirective_EPartsToSave_INT_MAX_SENTINEL_DO_NOT_USE_: {
            Metrics.PushRate("directive", "invalid", "save-user-audio");
            ythrow yexception() << "Invalid/unimplemented parts to save: " << NAlice::NScenarios::TSaveUserAudioDirective_EPartsToSave_Name(directive.GetPartsToSave());
        }
    }

    // Construct http request
    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Put);
    req.SetPath(TStringBuilder() << "/" << directive.GetStorageOptions().GetS3Storage().GetPath());
    AddHeader(req, "Host", TStringBuilder() << directive.GetStorageOptions().GetS3Storage().GetBucket() << '.' << NAws::S3_YANDEX_INTERNAL_HOST);
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostS3SaveUserAudioHttpRequest>(audioData.size(), req.ShortUtf8DebugString());

    // Do not log content and signature
    req.SetContent(TString(audioData));
    try {
        NAws::GetDefaultS3RequestSignerInstance().SignRequest(
            NAws::S3_YANDEX_INTERNAL_HOST,
            req
        );
    } catch (...) {
        Metrics.PushRate("directive", "signfailed", "save-user-audio");
        ythrow yexception() << "Failed to sign s3 request" << CurrentExceptionMessage();
    }

    Ctx.AddProtobufItem(req, ITEM_TYPE_S3_SAVE_USER_AUDIO_HTTP_REQUEST);
    Metrics.PushRate("directive", "ok", "save-user-audio");
}

void TContextSaveProcessor::PrePatchAsrOptionsForNextRequestDirective(const google::protobuf::Struct& payload) {
    if (RequestCtx.GetHeader().GetReqId().empty()) {
        Metrics.PushRate("directive", "empty_request_id", "patch-asr-options-for-next-request-directive");
        ythrow yexception() << "Request id is empty";
    }

    NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective directive = NAlice::StructToMessage<NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective>(payload);

    auto cachalotSetRequest = TCachalotCache::MakeSetRequest(RequestCtx.GetHeader().GetReqId(), "", TString(ASR_OPTIONS_STORAGE_TAG));
    LogContext.LogEventInfoCombo<NEvClass::SendToAppHostCachalotSaveAsrOptionsPatchRequest>(cachalotSetRequest.ShortUtf8DebugString(), directive.ShortUtf8DebugString());
    // Set data only after log
    cachalotSetRequest.SetData(directive.SerializeAsString());

    Ctx.AddProtobufItem(cachalotSetRequest, ITEM_TYPE_CACHALOT_SAVE_ASR_OPTIONS_PATCH_REQUEST);
    Ctx.AddBalancingHint(CACHALOT_SAVE_ASR_OPTIONS_PATCH_SOURCE, CityHash64(cachalotSetRequest.GetKey()));

    Metrics.PushRate("directive", "ok", "patch-asr-options-for-next-request-directive");
}

void TContextSaveProcessor::PreInvalidateDatasyncCache() {
    if (TDatasyncCache::IsEnabled(&RequestCtx) && HasAuthToken(&SessionCtx)) {
        TDatasyncCache(SessionCtx.GetUserInfo().GetAuthToken(), Ctx, LogContext, Metrics).Clear();
    }
}

}

void ContextSavePre(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextSaveProcessor proc(ctx, std::move(logContext), "context_save_pre");
    proc.Pre();
}

void ContextSavePost(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextSaveProcessor proc(ctx, std::move(logContext), "context_save_post");
    proc.Post();
}

void FakeContextSave(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextSaveProcessor proc(ctx, std::move(logContext), "fake_context_save");
    proc.Fake();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
