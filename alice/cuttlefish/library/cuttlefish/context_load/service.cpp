#include "cache.h"
#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/antirobot.h>
#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/datasync.h>
#include <alice/cuttlefish/library/cuttlefish/common/field_getters.h>
#include <alice/cuttlefish/library/cuttlefish/common/http_utils.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <alice/cuttlefish/library/cuttlefish/synchronize_state/flags_json.h>
#include <alice/cuttlefish/library/cuttlefish/synchronize_state/laas.h>
#include <alice/cuttlefish/library/experiments/flags_json.h>

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>
#include <alice/cuttlefish/library/proto_censor/context.h>

#include <alice/iot/bulbasaur/protos/apphost/iot.pb.h>
#include <alice/library/cachalot_cache/cachalot_cache.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/protos/api/meta/backend.pb.h>

#include <voicetech/library/messages/traits.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/lib/proto_answers/tvm_user_ticket.pb.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/openssl/crypto/sha.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <mssngr/router/lib/protos/registry/alice.pb.h>

#include <google/protobuf/util/json_util.h>

#include <util/digest/city.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/string/hex.h>


namespace NMemento = ru::yandex::alice::memento::proto;

using namespace NAliceProtocol;
using namespace NAlice::NAppHostServices;

namespace NAlice::NCuttlefish::NAppHostServices {

namespace NSupportedFeatures {

static constexpr TStringBuf NOTIFICATIONS = "notifications";

} // NSupportedFeatures

namespace {

const TString& GetMementoSurfaceId(const TSessionContext& ctx) {
    if (ctx.GetClientType() == TSessionContext::CLIENT_TYPE_QUASAR) {
        return ctx.GetDeviceInfo().GetDeviceId();
    } else {
        return GetUuid(ctx);
    }
}

NMemento::TReqGetAllObjects CreateMementoGetAllObjectsRequest(const TSessionContext& ctx) {
    NMemento::TReqGetAllObjects mementoReq;
    mementoReq.AddSurfaceId(GetMementoSurfaceId(ctx));
    return mementoReq;
}

TMaybe<NAliceProtocol::TSessionContext> TryLoadSessionContext(const NAppHost::IServiceContext& ctx) {
    NAliceProtocol::TRequestContext requestCtx;
    if (ctx.HasProtobufItem(ITEM_TYPE_SESSION_CONTEXT)) {
        return ctx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT);
    }
    return Nothing();
}

TMaybe<NAliceProtocol::TRequestContext> TryLoadRequestContext(const NAppHost::IServiceContext& ctx) {
    NAliceProtocol::TRequestContext requestCtx;
    if (ctx.HasProtobufItem(ITEM_TYPE_REQUEST_CONTEXT)) {
        return ctx.GetOnlyProtobufItem<NAliceProtocol::TRequestContext>(ITEM_TYPE_REQUEST_CONTEXT);
    }
    return Nothing();
}

bool FillCoordinates(
    const NAlice::TIoTUserInfo& iotUserInfo,
    TStringBuf deviceId,
    NAliceProtocol::TUserInfo* userInfo
) {
    for (const NAlice::TIoTUserInfo::TDevice& device : iotUserInfo.GetDevices()) {
        if ((device.GetQuasarInfo().GetDeviceId() == deviceId) && (device.GetSkillId() == "Q")) {
            for (const NAlice::TIoTUserInfo::THousehold& household : iotUserInfo.GetHouseholds()) {
                if (household.GetId() == device.GetHouseholdId()) {
                    if (!(FuzzyEquals(household.GetLongitude(), 0.0) && FuzzyEquals(household.GetLatitude(), 0.0))) {
                        userInfo->SetLongitude(household.GetLongitude());
                        userInfo->SetLatitude(household.GetLatitude());
                        return true;
                    }
                    break;
                }
            }
            break;
        }
    }

    return false;
}

TString TryBuildMementoCacheKey(const TSessionContext* ctx) {
    if (HasAuthToken(ctx)) {
        return TStringBuilder() << GetMementoSurfaceId(*ctx) << '|' << ctx->GetUserInfo().GetAuthToken();
    }
    // empty key means no request to cache
    return {};
}

TString TryBuildIotUserInfoCacheKey(const TSessionContext* sessionCtx, NAppHost::IServiceContext& ahCtx) {
    if (HasAuthToken(sessionCtx)) {
        const TString& authToken = sessionCtx->GetUserInfo().GetAuthToken();
        if (ahCtx.HasProtobufItem(ITEM_TYPE_SMARTHOME_UID)) {
            const auto& uid = ahCtx.GetOnlyProtobufItem<TContextLoadSmarthomeUid>(ITEM_TYPE_SMARTHOME_UID);
            return TStringBuilder() << authToken << '|' << uid.GetValue();
        }
        return authToken;
    }
    // empty key means no request to cache
    return {};
}


class TContextLoadProcessor {
public:
    TContextLoadProcessor(
        NAppHost::IServiceContext& ctx,
        TLogContext&& logContext,
        TStringBuf sourceName
    )
        : AhContext_(ctx)
        , LogContext_(logContext)
        , Metrics_(ctx, sourceName)
        , SessionCtx_(TryLoadSessionContext(ctx))
        , RequestCtx_(TryLoadRequestContext(ctx))
        , BlackboxResponseParser(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE)
    {}

    void Pre();
    void Post();
    void BlackboxSetdown();
    void ContactsRequest(TStringBuf, bool);
    bool ShouldFilterContacts();
    void MakeContactsRequest();
    void PrepareLaas();
    void PrepareFlagsJson();
    void Fake();

private:
    void TryPutTvmUserTicketToAppHostContext(const TBlackboxClient::TOAuthResponse& blackboxResp);
    void TryPutBlackboxUid(const TBlackboxClient::TOAuthResponse& blackboxResp);
    void EnsureSessionContextExist();

private:
    NAppHost::IServiceContext& AhContext_;
    TLogContext LogContext_;
    TSourceMetrics Metrics_;

    TMaybe<NAliceProtocol::TSessionContext> SessionCtx_;
    TMaybe<NAliceProtocol::TRequestContext> RequestCtx_;
    TBlackboxResponseParser BlackboxResponseParser;
};

void TContextLoadProcessor::Pre() {
    Y_ENSURE(SessionCtx_.Defined(), "Session context not provided");

    const TString& uuid = GetUuid(*SessionCtx_);
    LogContext_.LogEventInfoCombo<NEvClass::UUID>(uuid);

    const auto& userInfo = SessionCtx_->GetUserInfo();
    const auto& connInfo = SessionCtx_->GetConnectionInfo();

    TString sessionId;
    if (connInfo.HasCookie()) {
        sessionId = ExtractSessionIdFromCookie(connInfo.GetCookie());
    } else if (connInfo.HasXYambCookie()) {
        sessionId = ExtractSessionIdFromCookie(connInfo.GetXYambCookie());
    }

    // Construct BLACKBOX request
    if (userInfo.HasAuthToken() && connInfo.HasIpAddress()) {
        Metrics_.PushRate("request", "oauth_token", "blackbox");

        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxHttpRequest>("oauth_token");
        // construct requests to Blackbox
        AhContext_.AddProtobufItem(
            TBlackboxClient::GetUidForOAuth(userInfo.GetAuthToken(), connInfo.GetIpAddress()),
            ITEM_TYPE_BLACKBOX_HTTP_REQUEST
        );
    } else if (!sessionId.empty()) {
        Metrics_.PushRate("request", "session_id", "blackbox");

        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxHttpRequest>("session_id");
        // construct requests to Blackbox
        AhContext_.AddProtobufItem(
            TBlackboxClient::GetUidForSessionId(sessionId, connInfo.GetIpAddress(), connInfo.GetOrigin()),
            ITEM_TYPE_BLACKBOX_HTTP_REQUEST
        );
    } else {
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(
            TStringBuilder()
                << "Request to blackbox skipped: "
                << "oauth token is " << (userInfo.HasAuthToken() ? "set" : "empty")
                << ", ip address is " << (connInfo.HasIpAddress() ? "set" : "empty")
                << ", session id is " << (sessionId.empty() ? "empty" : "set")
        );
        if (!userInfo.HasAuthToken()) {
            Metrics_.PushRate("oauth_token", "empty");
        }
        if (!connInfo.HasIpAddress()) {
            Metrics_.PushRate("ip_address", "empty");
        }
        if (sessionId.empty()) {
            Metrics_.PushRate("session_id", "empty");
        }
    }

    //  Construct ANTIROBOT request
    if (AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS) && AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_INPUT_DATA)) {
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Has ANTIROBOT_INPUT_*" << uuid);
        const auto& settings = AhContext_.GetOnlyProtobufItem<TAntirobotInputSettings>(ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS);
        const auto& data = AhContext_.GetOnlyProtobufItem<TAntirobotInputData>(ITEM_TYPE_ANTIROBOT_INPUT_DATA);

        TMaybe<NAppHostHttp::THttpRequest> request = TAntirobotClient::CreateRequest(*SessionCtx_, settings, data);
        if (request) {
            Metrics_.PushRate("antirobot", (settings.GetMode() == EAntirobotMode::EVALUATE) ? "evaluate" : "apply");
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostAntirobotHttpRequest>();
            AhContext_.AddProtobufItem(request.GetRef(), ITEM_TYPE_ANTIROBOT_HTTP_REQUEST);
            AhContext_.AddBalancingHint("ANTIROBOT", CityHash64(data.GetForwardedFor()));
        } else {
            Metrics_.PushRate("antirobot", "off");
        }
    } else {
        if (!AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_INPUT_DATA)) {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "No ANTIROBOT_INPUT_DATA for uuid=" << uuid);
        }
        if (!AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS)) {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "No ANTIROBOT_INPUT_SETTINGS for uuid=" << uuid);
        }
        Metrics_.PushRate("antirobot", "nodata");
    }

    // Try load Datasync data from Cache
    if (TDatasyncCache::IsEnabled(RequestCtx_.Get()) && HasAuthToken(SessionCtx_.Get())) {
        TDatasyncCache(userInfo.GetAuthToken(), AhContext_, LogContext_, Metrics_).Load();
    }

    // Try load Memento data from Cache
    if (TMementoCache::IsEnabled(RequestCtx_.Get())) {
        if (TString authToken = TryBuildMementoCacheKey(SessionCtx_.Get())) {
            TMementoCache(std::move(authToken), AhContext_, LogContext_, Metrics_).Load();
        }
    }

    // Try load IoTUserInfo data from Cache
    if (TIoTUserInfoCache::IsEnabled(RequestCtx_.Get())) {
        if (TString authToken = TryBuildIotUserInfoCacheKey(SessionCtx_.Get(), AhContext_)) {
            TIoTUserInfoCache(std::move(authToken), AhContext_, LogContext_, Metrics_).Load();
        }
    }

    if (RequestCtx_.Defined() && RequestCtx_->HasHeader() && !RequestCtx_->GetHeader().GetPrevReqId().empty()) {
        TString key = RequestCtx_->GetHeader().GetPrevReqId();
        auto request = TCachalotCache::MakeGetRequest(key, "AsrOptions");
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostCachalotLoadAsrOptionsPatchRequest>(request.ShortUtf8DebugString());

        AhContext_.AddBalancingHint("CACHALOT_LOAD_ASR_OPTIONS_PATCH", CityHash64(key));
        AhContext_.AddProtobufItem(request, ITEM_TYPE_CACHALOT_LOAD_ASR_OPTIONS_PATCH_REQUEST);
    }
}

void TContextLoadProcessor::Post() {
    TContextLoadResponse response;

    // Set Memento response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_MEMENTO_USER_OBJECTS)) {
        auto mementoResponse = AhContext_.GetOnlyProtobufItem<NMemento::TRespGetAllObjects>(
            ITEM_TYPE_MEMENTO_USER_OBJECTS
        );
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostMementoResponse>(
            mementoResponse.ShortUtf8DebugString()
        );
        response.MutableMementoResponse()->SetContent(mementoResponse.SerializeAsString());

        Metrics_.PushRate("response", "ok", "memento");

        if (TString key = TryBuildMementoCacheKey(SessionCtx_.Get())) {
            const NAppHostHttp::THttpResponse& cacheItem = response.GetMementoResponse();
            TMementoCache(std::move(key), AhContext_, LogContext_, Metrics_).Store(cacheItem);
        }
    } else {
        if (AhContext_.HasProtobufItem(ITEM_TYPE_MEMENTO_GET_ALL_OBJECTS_REQUEST)) {
            LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Memento");
            Metrics_.PushRate("response", "noans", "memento");
        }
    }
    if (!response.HasMementoResponse()) {
        if (auto rsp = TMementoCache("TODO_paxakor", AhContext_, LogContext_, Metrics_).TryParseLoadedData()) {
            // TODO (paxakor): change type of proto stored in cache
            NMemento::TRespGetAllObjects mementoResponse;
            if (mementoResponse.ParseFromString(rsp.GetRef().GetContent())) {
                AhContext_.AddProtobufItem(mementoResponse, ITEM_TYPE_MEMENTO_USER_OBJECTS);
            }

            response.MutableMementoResponse()->Swap(rsp.Get());
        }
    }

    // Set Contacts response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS)) {
        auto item = AhContext_.GetOnlyProtobufItem<TContextLoadPredefinedContacts>(ITEM_TYPE_PREDEFINED_CONTACTS);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostPredefinedContacts>(item.GetValue());
        NAppHostHttp::THttpResponse *cts = response.MutableContactsResponse();
        cts->SetStatusCode(200);
        cts->SetContent(item.GetValue());
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_CONTACTS_PROTO_HTTP_RESPONSE)) {
        Metrics_.PushRate("response", "ok", "contacts-proto");
        auto item = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_CONTACTS_PROTO_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostContactsHttpResponse>(item.ShortUtf8DebugString());
        NRegistryProtocol::TListContactsResponse resp;
        if (resp.ParseFromString(item.GetContent())) {
            response.MutableContactsProto()->Swap(resp.MutableContactsList());
        } else {
            LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("Failed to parse contacts answer");
            Metrics_.PushRate("response", "noans", "contacts-proto");
        }
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_CONTACTS_PROTO_HTTP_REQUEST)) {
        response.MutableContactsProto()->MutableContacts();
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Contacts");
        Metrics_.PushRate("response", "noans", "contacts-proto");
    }

    // Handle antirobot response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS)) {
        auto settings = AhContext_.GetOnlyProtobufItem<TAntirobotInputSettings>(ITEM_TYPE_ANTIROBOT_INPUT_SETTINGS);

        if (settings.GetMode() != EAntirobotMode::OFF) {
            if (AhContext_.HasProtobufItem(ITEM_TYPE_ANTIROBOT_HTTP_RESPONSE)) {
                Metrics_.PushRate("response", "ok", "antirobot");
                auto item = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_ANTIROBOT_HTTP_RESPONSE);

                TRobotnessData out;
                if (TAntirobotClient::ParseResponseTo(item, &out)) {
                    if (out.GetIsRobot()) {
                        Metrics_.PushRate("robotness", "robot", "antirobot");
                    } else {
                        Metrics_.PushRate("robotness", "user", "antirobot");
                    }

                    if (settings.GetMode() == EAntirobotMode::APPLY) {
                        response.MutableRobotness()->CopyFrom(out);
                    } else {
                        response.MutableRobotness()->SetIsRobot(false);
                        response.MutableRobotness()->SetRobotness(0.0);
                    }
                } else {
                    Metrics_.PushRate("robotness", "error", "antirobot");
                }
            } else {
                Metrics_.PushRate("response", "noans", "antirobot");
            }
        }
    }

    // Datasync
    if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_HTTP_RESPONSE)) {
        NAppHostHttp::THttpResponse datasyncResponse = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(
            ITEM_TYPE_DATASYNC_HTTP_RESPONSE
        );
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostDatasyncHttpResponse>(
            datasyncResponse.ShortUtf8DebugString()
        );

        if (datasyncResponse.GetStatusCode() / 100 == 2) {
            Metrics_.PushRate("response", "ok", "datasync");
            response.MutableDatasyncResponse()->Swap(&datasyncResponse);

            if (HasAuthToken(SessionCtx_.Get())) {
                const TString& authToken = SessionCtx_->GetUserInfo().GetAuthToken(); // used as a cache key
                const NAppHostHttp::THttpResponse& cacheItem = response.GetDatasyncResponse();
                TDatasyncCache(authToken, AhContext_, LogContext_, Metrics_).Store(cacheItem);
            }
        } else {
            Metrics_.PushRate("response", "error", "datasync");
        }
    } else {
        if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_HTTP_REQUEST)) {
            Metrics_.PushRate("response", "noans", "datasync");
        }
    }
    if (!response.HasDatasyncResponse()) {
        if (auto rsp = TDatasyncCache("TODO_paxakor", AhContext_, LogContext_, Metrics_).TryParseLoadedData()) {
            response.MutableDatasyncResponse()->Swap(rsp.Get());
        }
    }

    // Set DatasyncDeviceId response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_RESPONSE)) {
        Metrics_.PushRate("response", "ok", "datasync-device");
        auto item = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostDatasyncDeviceIdHttpResponse>(item.ShortUtf8DebugString());
        response.MutableDatasyncDeviceIdResponse()->Swap(&item);
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_REQUEST)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Datasync DeviceId");
        Metrics_.PushRate("response", "noans", "datasync-device");
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_UUID_HTTP_RESPONSE)) {
        Metrics_.PushRate("response", "ok", "datasync-uuid");
        auto item = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_DATASYNC_UUID_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostDatasyncUuidHttpResponse>(item.ShortUtf8DebugString());
        response.MutableDatasyncUuidResponse()->Swap(&item);
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_DATASYNC_UUID_HTTP_REQUEST)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Datasync UUID");
        Metrics_.PushRate("response", "noans", "datasync-uuid");
    }

    // Set QuasarIot response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_QUASARIOT_RESPONSE_IOT_USER_INFO)) {
        Metrics_.PushRate("response", "ok", "quasar-iot");
        auto item = AhContext_.GetOnlyProtobufItem<NAlice::TIoTUserInfo>(ITEM_TYPE_QUASARIOT_RESPONSE_IOT_USER_INFO);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostQuasarIotUserInfo>(
            Base64Encode(item.ShortUtf8DebugString()));

        if (TString authToken = TryBuildIotUserInfoCacheKey(SessionCtx_.Get(), AhContext_)) {
            TIoTUserInfoCache(std::move(authToken), AhContext_, LogContext_, Metrics_).Store(item);
        }

        response.MutableIoTUserInfo()->Swap(&item);
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_TVM_USER_TICKET)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from IotUserInfo");
        Metrics_.PushRate("response", "noans", "quasar-iot");

        if (auto rsp = TIoTUserInfoCache("TODO_paxakor", AhContext_, LogContext_, Metrics_).TryParseLoadedData()) {
            response.MutableIoTUserInfo()->Swap(rsp.Get());
        }
    }

    // Set Notificator response
    if (AhContext_.HasProtobufItem(ITEM_TYPE_NOTIFICATOR_HTTP_RESPONSE)) {
        Metrics_.PushRate("response", "ok", "notificator");
        auto item = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_NOTIFICATOR_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostNotificatorHttpResponse>(item.ShortUtf8DebugString());
        response.MutableNotificatorResponse()->Swap(&item);
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_NOTIFICATOR_HTTP_REQUEST)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Notificator");
        Metrics_.PushRate("response", "noans", "notificator");
    }

    // Set Megamind session response (from cachalot)
    if (AhContext_.HasProtobufItem(ITEM_TYPE_MEGAMIND_SESSION_RESPONSE)) {
        auto item = AhContext_.GetOnlyProtobufItem<NCachalotProtocol::TResponse>(ITEM_TYPE_MEGAMIND_SESSION_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostMegamindSessionResponse>();
        response.MutableMegamindSessionResponse()->Swap(&item);

        {
            const auto& session = response.GetMegamindSessionResponse().GetMegamindSessionLoadResp();
            if (session.HasData() && session.GetData().size() > 0) {
                Metrics_.PushRate("response", "ok", "cachalot-mm");
            } else {
                Metrics_.PushRate("response", "empty", "cachalot-mm");
            }
        }
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_MEGAMIND_SESSION_REQUEST)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Cachalot");
        Metrics_.PushRate("response", "noans", "cachalot-mm");
    }

    // Set User ticket (from BlackBox response)
    if (AhContext_.HasProtobufItem(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE)) {
        const auto& blackboxHttpResp = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostBlackboxHttpResponse>();
        // in case of non-200 code or noans, error metric already incremented in ContextLoadBlackboxSetdown
        if (blackboxHttpResp.GetStatusCode() == 200) {
            TBlackboxClient::TOAuthResponse blackboxResp = TBlackboxClient::ParseResponse(blackboxHttpResp.GetContent());
            if (blackboxResp.Valid) {
                response.SetUserTicket(blackboxResp.UserTicket);
                response.SetBlackboxUid(blackboxResp.Uid);
            }
        }

        Metrics_.PushRate("response", (response.HasUserTicket() && response.HasBlackboxUid()) ? "ok" : "error", "blackbox");
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_BLACKBOX_HTTP_REQUEST)) {
        Metrics_.PushRate("response", "noans", "blackbox");
    }

    TMaybe<NAliceProtocol::TAbFlagsProviderOptions> options = Nothing();
    if (AhContext_.HasProtobufItem(ITEM_TYPE_AB_EXPERIMENTS_OPTIONS)) {
        options = AhContext_.GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
    }

    if (AhContext_.HasItem(ITEM_TYPE_FLAGS_JSON_HTTP_RESPONSE)) {
        const auto rawItemRefs = AhContext_.GetRawItemRefs(
            ITEM_TYPE_FLAGS_JSON_HTTP_RESPONSE,
            NAppHost::EContextItemSelection::RawInput
        );

        NAppHostHttp::THttpResponse fjHttpRsp;

        if (rawItemRefs.ysize() == 1) {
            const TString rawFjResponse = rawItemRefs.front().data();
            LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostFlagsJsonHttpResponse>(
                200, rawFjResponse
            );

            NAliceProtocol::TFlagsInfo flagsInfo;
            NVoice::NExperiments::ParseFlagsInfoFromRawResponse(&flagsInfo, rawFjResponse);
            TFlagsJson::CountFlags(flagsInfo, options, &Metrics_, &LogContext_);

            try {  // SESSION_LOG with FlagsJson record
                NAliceProtocol::TUniproxyDirective directive;
                NAliceProtocol::TSessionLogRecord& sessionLog = *directive.MutableSessionLog();
                NJson::TJsonValue sessionLogValue;
                sessionLogValue["type"] = "FlagsJson";
                sessionLogValue["Body"]["test_ids"] = NJson::TJsonArray();
                for (const TString& testId : flagsInfo.GetAllTestIds()) {
                    sessionLogValue["Body"]["test_ids"].AppendValue(testId);
                }
                sessionLog.SetName("Directive");
                sessionLog.SetValue(NJson::WriteJson(sessionLogValue, false, true));
                sessionLog.SetAction("log_flags_json");
                LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostDirective>(directive.ShortUtf8DebugString());
                AhContext_.AddProtobufItem(directive, NCuttlefish::ITEM_TYPE_UNIPROXY2_DIRECTIVE);
            } catch (...) {
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(
                    TStringBuilder() <<
                        "Failed to build SessionLog message for flags json: " <<
                        CurrentExceptionMessage()
                );
            }

            AhContext_.AddProtobufItem(flagsInfo, ITEM_TYPE_FLAGS_INFO);
            // TODO (paxakor): deduplicate.
            response.MutableFlagsInfo()->Swap(&flagsInfo);

            fjHttpRsp.SetStatusCode(200);
            fjHttpRsp.SetContent(rawFjResponse);
        } else {
            fjHttpRsp.SetStatusCode(400);
            Metrics_.PushRate("response", "error", "flags_json");
        }
        response.MutableFlagsJsonResponse()->Swap(&fjHttpRsp);
    } else if (options && !options->GetDisregardUaas()) {
        Metrics_.PushRate("response", "noans", "flags_json");
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_LAAS_HTTP_RESPONSE)) {
        auto laasRsp = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_LAAS_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostLaasHttpResponse>(
            laasRsp.GetStatusCode(), laasRsp.GetContent()
        );
        if (laasRsp.GetStatusCode() == 200) {
            Metrics_.PushRate("response", "ok", "laas");
            response.MutableLaasResponse()->Swap(&laasRsp);
        } else {
            Metrics_.PushRate("response", "error", "laas");
        }
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_LAAS_HTTP_REQUEST)) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from Laas");
        Metrics_.PushRate("response", "noans", "laas");
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_CACHALOT_LOAD_ASR_OPTIONS_PATCH_RESPONSE)) {
        auto cachalotResponse = AhContext_.GetOnlyProtobufItem<NCachalotProtocol::TResponse>(ITEM_TYPE_CACHALOT_LOAD_ASR_OPTIONS_PATCH_RESPONSE);
        const auto status = cachalotResponse.GetStatus();

        if (status == NCachalotProtocol::OK) {
            TString data = "";
            // Try to log only parsed data
            cachalotResponse.MutableGetResp()->MutableData()->swap(data);;

            NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective patchAsrOptionsForNextRequestDirective;
            if (patchAsrOptionsForNextRequestDirective.ParseFromString(data)) {
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostCachalotLoadAsrOptionsPatchResponse>(
                    cachalotResponse.ShortUtf8DebugString(),
                    patchAsrOptionsForNextRequestDirective.ShortUtf8DebugString()
                );
                Metrics_.PushRate("response", "ok", "cachalot-asr-options");

                response.MutablePatchAsrOptionsForNextRequestDirective()->Swap(&patchAsrOptionsForNextRequestDirective);
            } else {
                LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostCachalotLoadAsrOptionsPatchResponse>(cachalotResponse.ShortUtf8DebugString(), data);
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Failed to parse asr options patch from cachalot response");
                Metrics_.PushRate("response", "parse-error", "cachalot-asr-options");
            }
        } else if (status == NCachalotProtocol::NO_CONTENT) {
            LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostCachalotLoadAsrOptionsPatchResponse>(cachalotResponse.ShortUtf8DebugString(), "");
            Metrics_.PushRate("response", "nodata", "cachalot-asr-options");
        } else {
            LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostCachalotLoadAsrOptionsPatchResponse>(cachalotResponse.ShortUtf8DebugString(), "");
            Metrics_.PushRate("response", "error", "cachalot-asr-options");
        }

        // TODO(VOICESERV-4184) fix this "else if"
    } else if (RequestCtx_.Defined() && RequestCtx_->HasHeader() && !RequestCtx_->GetHeader().GetPrevReqId().empty()) {
        LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>("No answer from asr options cachalot");
        Metrics_.PushRate("response", "noans", "cachalot-asr-options");
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE)) {
        *response.MutableGuest()->MutableBlackboxResponse() = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_GUEST_BLACKBOX_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostGuestBlackboxResponse>();
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE)) {
        *response.MutableGuest()->MutableDatasyncResponse() = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_GUEST_DATASYNC_HTTP_RESPONSE);
        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostGuestDatasyncResponse>();
    }

    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostContextLoadResponse>(CensoredContextLoadResponseStr(response));
    AhContext_.AddProtobufItem(response, ITEM_TYPE_CONTEXT_LOAD_RESPONSE);
}

void TContextLoadProcessor::ContactsRequest(TStringBuf blackboxUid, bool doFilter) {
    const bool hasPredefinedContacts = AhContext_.HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS);

    if (!hasPredefinedContacts && !blackboxUid.Empty()) {
        // construct request to contacts
        NAppHostHttp::THttpRequest protoReq;
        protoReq.SetPath("/meta_api/list_contacts_alice");
        protoReq.SetMethod(NAppHostHttp::THttpRequest::Post);
        AddHeader(protoReq, "Content-Type", "application/octet-stream");
        if (RequestCtx_.Defined()) {
            if (RequestCtx_->HasHeader() && RequestCtx_->GetHeader().HasMessageId()) {
                const TString& id = CGIEscapeRet(RequestCtx_->GetHeader().GetMessageId());
                AddHeader(protoReq, "X-Request-Id", id);
            }
        }

        NRegistryProtocol::TListContactsRequest req;
        if (uint64_t uid = 0; TryFromString(blackboxUid, uid)) {
            req.SetUid(uid);
            req.SetUuid(GetUuid(*SessionCtx_));
            req.SetFilter(doFilter);
            bool didSer = req.SerializeToString(protoReq.MutableContent());
            Y_ASSERT(didSer);

            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostContactsHttpRequest>(protoReq.ShortUtf8DebugString());
            AhContext_.AddProtobufItem(protoReq, ITEM_TYPE_CONTACTS_PROTO_HTTP_REQUEST);

            Metrics_.PushRate("request", "ok", "contacts");
        } else {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "blackbox uid is not an int");
            Metrics_.PushRate("request", "other-error", "contacts-proto");
        }
    } else if (hasPredefinedContacts) {
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Predefined contacts specified, skipping request");
    } else {
        Metrics_.PushRate("request", "no-uid", "contacts");
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Puid does not exist");
    }
}

bool TContextLoadProcessor::ShouldFilterContacts() {
    if (AhContext_.HasItem(ITEM_TYPE_FLAGS_JSON_HTTP_RESPONSE)) {
        const NJson::TJsonValue fjRsp = AhContext_.GetOnlyItem(ITEM_TYPE_FLAGS_JSON_HTTP_RESPONSE);
        NAliceProtocol::TFlagsInfo flagsInfo;
        NVoice::NExperiments::ParseFlagsInfoFromJsonResponse(&flagsInfo, fjRsp);
        return NVoice::NExperiments::TFlagsJsonFlagsConstRef(&flagsInfo).ConductingExperiment("filter_contacts");
    }
    return false;
}

void TContextLoadProcessor::MakeContactsRequest() {
    TMaybe<TBlackboxClient::TOAuthResponse> maybeBlackbox = BlackboxResponseParser.TryParse(AhContext_, Metrics_, LogContext_);
    if (!maybeBlackbox) {
        return;
    }
    ContactsRequest(maybeBlackbox->Uid, ShouldFilterContacts());
}

void TContextLoadProcessor::TryPutTvmUserTicketToAppHostContext(const TBlackboxClient::TOAuthResponse& blackboxResp) {
    if (!blackboxResp.UserTicket) {
        LogContext_.LogEventInfoCombo<NEvClass::NoTvmUserTicketInBlackboxResponse>();
    } else {
        NAppHostTvmUserTicket::TTvmUserTicket tvmUserTicket;
        tvmUserTicket.SetUserTicket(blackboxResp.UserTicket);

        // TODO (paxakor): deduplicate after release
        AhContext_.AddProtobufItem(tvmUserTicket, ITEM_TYPE_TVM_USER_TICKET);
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTvmUserTicket>();
    }
}

void TContextLoadProcessor::TryPutBlackboxUid(const TBlackboxClient::TOAuthResponse& blackboxResp) {
    if (!blackboxResp.Uid) {
        LogContext_.LogEventInfoCombo<NEvClass::NoUidInBlackboxResponse>();
        return;
    }

    NAliceProtocol::TContextLoadBlackboxUid blackboxUid;
    blackboxUid.SetUid(blackboxResp.Uid);

    AhContext_.AddProtobufItem(blackboxUid, ITEM_TYPE_BLACKBOX_UID);
    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxUid>();
}

void TContextLoadProcessor::BlackboxSetdown() {
    Y_ENSURE(SessionCtx_.Defined(), "Session context not provided");

    const TString& uuid = GetUuid(*SessionCtx_);
    TMaybe<TBlackboxClient::TOAuthResponse> maybeBlackbox = BlackboxResponseParser.TryParse(AhContext_, Metrics_, LogContext_);

    {
        NAlice::TLoggerOptions aliceLoggerOptions;

        if (
            (maybeBlackbox.Defined() && bool(maybeBlackbox->StaffLogin)) ||
            LogContext_.Options().WriteInfoToRtLog ||
            NVoicetech::NUniproxy2::GetUuidKind(uuid) != NVoicetech::NUniproxy2::EUuidKind::User
        ) {
            aliceLoggerOptions.SetSetraceLogLevel(NAlice::ELogLevel::ELL_INFO);
        } else {
            aliceLoggerOptions.SetSetraceLogLevel(NAlice::ELogLevel::ELL_ERROR);
        }

        AhContext_.AddProtobufItem(std::move(aliceLoggerOptions), ITEM_TYPE_ALICE_LOGGER_OPTIONS);
    }

    if (!maybeBlackbox) {
        if (!uuid.Empty()) {
            Metrics_.PushRate("uuid", "ok");
            {
                // construct requests to Datasync with device_id
                auto req = TDatasyncClient::LoadVinsContextsRequest();
                TDatasyncClient::AddDeviceIdHeader(req, uuid);

                LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncDeviceIdHttpRequest>(req.ShortUtf8DebugString());
                AhContext_.AddProtobufItem(req, ITEM_TYPE_DATASYNC_DEVICE_ID_HTTP_REQUEST);
            }
            {
                // construct requests to Datasync with uuid
                auto req = TDatasyncClient::LoadVinsContextsRequest();
                TDatasyncClient::AddUuidHeader(req, uuid);

                LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncUuidHttpRequest>(req.ShortUtf8DebugString());
                AhContext_.AddProtobufItem(req, ITEM_TYPE_DATASYNC_UUID_HTTP_REQUEST);
            }
        } else {
            Metrics_.PushRate("uuid", "empty");
        }
        return;
    }

    const TBlackboxClient::TOAuthResponse& blackboxResp = *maybeBlackbox;
    TryPutTvmUserTicketToAppHostContext(blackboxResp);
    TryPutBlackboxUid(blackboxResp);

    TMaybe<TString> deviceId;
    TMaybe<TString> deviceModel;
    THashSet<TString> supportedFeatures;
    if (SessionCtx_->HasDeviceInfo()) {
        const auto& info = SessionCtx_->GetDeviceInfo();
        if (info.HasDeviceId()) {
            deviceId = info.GetDeviceId();
        }
        if (info.HasDeviceModel()) {
            deviceModel = info.GetDeviceModel();
        }
        if (const auto& supportedFeaturesList = info.GetSupportedFeatures(); !supportedFeaturesList.empty()) {
            supportedFeatures = THashSet<TString>(
                supportedFeaturesList.begin(),
                supportedFeaturesList.end()
            );
        }
    }

    {
        // construct request to Memento
        NAppHostHttp::THttpRequest req;
        req.SetPath("/memento/get_all_objects");
        req.SetMethod(NAppHostHttp::THttpRequest::Post);
        AddHeader(req, "Content-Type", "application/protobuf");
        auto mementoReq = CreateMementoGetAllObjectsRequest(*SessionCtx_);
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostMementoHttpRequest>(req.ShortUtf8DebugString(), mementoReq.ShortUtf8DebugString());
        req.SetContent(mementoReq.SerializeAsString());
        AddHeader(req, "X-Ya-User-Ticket", blackboxResp.UserTicket);

        AhContext_.AddProtobufItem(std::move(mementoReq), ITEM_TYPE_MEMENTO_GET_ALL_OBJECTS_REQUEST);

        Metrics_.PushRate("request", "ok", "memento");
    }

    ContactsRequest(blackboxResp.Uid, false);

    {  // construct request to Datasync
        NAppHostHttp::THttpRequest req = TDatasyncClient::LoadVinsContextsRequest();
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncHttpRequest>(req.ShortUtf8DebugString());
        TDatasyncClient::AddUserTicketHeader(req, blackboxResp.UserTicket);
        TDatasyncClient::AddUidHeader(req, blackboxResp.Uid);

        AhContext_.AddProtobufItem(req, ITEM_TYPE_DATASYNC_HTTP_REQUEST);

        Metrics_.PushRate("request", "ok", "datasync");
    }

    if (AhContext_.HasItem(ITEM_TYPE_PREDEFINED_IOT_CONFIG)) {
        LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>("Found item predefinedIotConfig");

        const auto predefinedIotConfigInfo = AhContext_.GetOnlyItem(ITEM_TYPE_PREDEFINED_IOT_CONFIG);
        if (predefinedIotConfigInfo["has_predefined_iot_config"].GetBoolean()) {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>("Cancel request to IoT: predefinedIotConfig");
        }
    } else {  // construct request to QuasarIot
        if (AhContext_.HasProtobufItem(ITEM_TYPE_SMARTHOME_UID)) {
            const auto& uid = AhContext_.GetOnlyProtobufItem<TContextLoadSmarthomeUid>(ITEM_TYPE_SMARTHOME_UID);
            if (uint64_t alice4BusinessUid = 0; TryFromString(uid.GetValue(), alice4BusinessUid)) {
                NIoTProtocol::TAlice4Business alice4Business;
                alice4Business.SetUid(alice4BusinessUid);
                AhContext_.AddProtobufItem(std::move(alice4Business), ITEM_TYPE_QUASARIOT_REQUEST_ALICE_FOR_BUSINESS);
                Metrics_.PushRate("request", "ok", "quasar-iot-business");
                LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostQuasarIotRequestA4BUid>(alice4BusinessUid);
            } else {
                Metrics_.PushRate("request", "error", "quasar-iot-business");
                LogContext_.LogEventInfoCombo<NEvClass::InvalidQuasarIotRequestA4BUid>(uid.GetValue());
            }
        } else {
            Metrics_.PushRate("request", "ok", "quasar-iot");
        }
    }

    if (deviceId.Defined() && deviceModel.Defined() && supportedFeatures.contains(NSupportedFeatures::NOTIFICATIONS)) {
        // construct request to Notificator
        NAppHostHttp::THttpRequest req;
        req.SetPath(TStringBuilder{} << "/notifications?puid=" << blackboxResp.Uid << "&device_id=" << *deviceId << "&device_model=" << *deviceModel);
        AddHeader(req, "Content-Type", "application/protobuf");

        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostNotificatorHttpRequest>(req.ShortUtf8DebugString());
        AhContext_.AddProtobufItem(req, ITEM_TYPE_NOTIFICATOR_HTTP_REQUEST);

        Metrics_.PushRate("request", "ok", "notificator");
    }
}

void TContextLoadProcessor::PrepareLaas() {
    Y_ENSURE(SessionCtx_.Defined(), "Session context not provided");

    bool gotCoordinatesFromPredefinedIotConfig = false;

    if (AhContext_.HasItem(ITEM_TYPE_PREDEFINED_IOT_CONFIG)) {
        const auto predefinedIotConfigInfo = AhContext_.GetOnlyItem(ITEM_TYPE_PREDEFINED_IOT_CONFIG);
        if (TString dump; predefinedIotConfigInfo["serialized_iot_config"].GetString(&dump)) {
            TString decoded;
            try {
                Base64StrictDecode(dump, decoded);
            } catch (...) {
                LogContext_.LogEventInfoCombo<NEvClass::UnableToParseBase64OfPredefinedIoT>(dump);
            }
            if (!decoded.empty()) {
                if (NAlice::TIoTUserInfo iotUserInfo; (!decoded.empty()) && iotUserInfo.ParseFromString(decoded)) {
                    const TStringBuf deviceId = SessionCtx_->GetDeviceInfo().GetDeviceId();
                    NAliceProtocol::TUserInfo* userInfo = SessionCtx_->MutableUserInfo();
                    if (FillCoordinates(iotUserInfo, deviceId, userInfo)) {
                        Metrics_.PushRate("response", "ok", "predefined-quasar-iot-for-laas");
                        LogContext_.LogEventInfoCombo<NEvClass::CoordinatesFoundInPredefinedIoT>(
                            userInfo->GetLongitude(), userInfo->GetLatitude()
                        );
                        gotCoordinatesFromPredefinedIotConfig = true;
                    }
                } else  {
                    LogContext_.LogEventInfoCombo<NEvClass::UnableToParseProtoOfPredefinedIoT>(decoded);
                }
            }
        }

        if (!gotCoordinatesFromPredefinedIotConfig) {
            Metrics_.PushRate("response", "no-data", "predefined-quasar-iot-for-laas");
            LogContext_.LogEventInfoCombo<NEvClass::CoordinatesNotFoundInPredefinedIoT>();
        }
    }

    if (gotCoordinatesFromPredefinedIotConfig) {
        // Then do not process real quasar-iot response.
    } else if (AhContext_.HasProtobufItem(ITEM_TYPE_QUASARIOT_RESPONSE_IOT_USER_INFO)) {
        auto iotUserInfo = AhContext_.GetOnlyProtobufItem<NAlice::TIoTUserInfo>(ITEM_TYPE_QUASARIOT_RESPONSE_IOT_USER_INFO);
        const TStringBuf deviceId = SessionCtx_->GetDeviceInfo().GetDeviceId();
        NAliceProtocol::TUserInfo* userInfo = SessionCtx_->MutableUserInfo();
        if (FillCoordinates(iotUserInfo, deviceId, userInfo)) {
            Metrics_.PushRate("response", "ok", "quasar-iot-for-laas");
            LogContext_.LogEventInfoCombo<NEvClass::CoordinatesFoundInIoTResponse>(
                userInfo->GetLongitude(), userInfo->GetLatitude()
            );
        } else {
            Metrics_.PushRate("response", "no-data", "quasar-iot-for-laas");
            LogContext_.LogEventInfoCombo<NEvClass::CoordinatesNotFoundInIoTResponse>();
        }
    } else /* Here we assume that request has been made */ {
        Metrics_.PushRate("response", "noans", "quasar-iot-for-laas");
        LogContext_.LogEventInfoCombo<NEvClass::NotFoundIoTResponseForPrepareLaas>();
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE)) {
        const auto bbRsp = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE);
        if (bbRsp.GetStatusCode() == 200) {
            TBlackboxClient::TOAuthResponse blackBoxResponse = TBlackboxClient::ParseResponse(bbRsp.GetContent());
            if (blackBoxResponse.Valid) {
                Metrics_.PushRate("response", "ok", "blackbox-for-laas");
                SessionCtx_->MutableUserInfo()->SetPuid(std::move(blackBoxResponse.Uid));
            } else {
                Metrics_.PushRate("response", "error", "blackbox-for-laas");
                LogContext_.LogEventInfoCombo<NEvClass::BlackBoxRejectedToken>();
            }
        } else {
            Metrics_.PushRate("response", "error", "blackbox-for-laas");
            LogContext_.LogEventInfoCombo<NEvClass::BlackBoxRequestFailed>(bbRsp.GetStatusCode());
        }
    } else /* Here we assume that request has been made */ {
        Metrics_.PushRate("response", "noans", "blackbox-for-laas");
        LogContext_.LogEventInfoCombo<NEvClass::NotFoundBlackboxResponseForPrepareLaas>();
    }

    NAliceProtocol::TContextLoadLaasRequestOptions options;
    if (AhContext_.HasProtobufItem(ITEM_TYPE_LAAS_REQUEST_OPTIONS)) {
        options = AhContext_.GetOnlyProtobufItem<NAliceProtocol::TContextLoadLaasRequestOptions>(
            ITEM_TYPE_LAAS_REQUEST_OPTIONS
        );
    }

    NAppHostHttp::THttpRequest req = TLaas::CreateRequest(*SessionCtx_, options);
    Metrics_.PushRate("request", "ok", "laas");
    LogContext_.LogEventInfoCombo<NEvClass::LaasRequest>(TStringBuilder() << req);
    AhContext_.AddProtobufItem(std::move(req), ITEM_TYPE_LAAS_HTTP_REQUEST);
}

void TContextLoadProcessor::PrepareFlagsJson() {
    Y_ENSURE(SessionCtx_.Defined(), "Session context not provided");

    NAliceProtocol::TAbFlagsProviderOptions options;
    if (AhContext_.HasProtobufItem(ITEM_TYPE_AB_EXPERIMENTS_OPTIONS)) {
        options = AhContext_.GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE)) {
        const auto& bbHttpResp = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE);
        const TBlackboxClient::TOAuthResponse blackboxResp = TBlackboxClient::ParseResponse(bbHttpResp.GetContent());
        if (blackboxResp.Valid) {
            options.SetPuid(blackboxResp.Uid);
            options.SetIsYandexStaff(bool(blackboxResp.StaffLogin));
            options.SetIsBetaTester(blackboxResp.IsBetaTester);
        }
    }

    if (AhContext_.HasProtobufItem(ITEM_TYPE_LAAS_HTTP_RESPONSE)) {
        const auto& laasRsp = AhContext_.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_LAAS_HTTP_RESPONSE);
        if (NJson::TJsonValue data; NJson::ReadJsonTree(laasRsp.GetContent(), &data, /*throwOnError = */ false)) {
            if (const NJson::TJsonValue regionId = data["region_id"]; regionId.IsInteger()) {
                options.SetRegionId(regionId.GetInteger());
            }
        }
    }

    bool shouldMakeFlagsJsonRequest = true;
    if (SessionCtx_->GetExperiments().GetOnly100PercentFlagsForSession() || options.GetOnly100PercentFlags()) {
        options.SetOnly100PercentFlags(true);
        LogContext_.LogEventInfoCombo<NEvClass::FoundFlagOnly100PercentFlagsForSession>();
    } else {
        if (SessionCtx_->GetExperiments().GetDisregardUaasForSession() || options.GetDisregardUaas()) {
            LogContext_.LogEventInfoCombo<NEvClass::FoundFlagDisregardUaasForSession>();
            shouldMakeFlagsJsonRequest = false;
        }
    }

    if (!shouldMakeFlagsJsonRequest) {
        return;
    }

    NJson::TJsonValue req = TFlagsJson::GetExperimentsJson(*SessionCtx_, options, Metrics_, LogContext_);
    LogContext_.LogEventInfoCombo<NEvClass::SendingFlagsJsonHttpRequestToAppHost>(TStringBuilder() << req);
    AhContext_.AddItem(std::move(req), ITEM_TYPE_FLAGS_JSON_HTTP_REQUEST);
}

void TContextLoadProcessor::Fake() {
    TContextLoadResponse response;
    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostContextLoadResponse>(CensoredContextLoadResponseStr(response));
    AhContext_.AddProtobufItem(response, ITEM_TYPE_CONTEXT_LOAD_RESPONSE);
}

}  // anonymous namespace

void ContextLoadPre(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_pre");
    proc.Pre();
}

void ContextLoadPost(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_post");
    proc.Post();
}

void ContextLoadMakeContactsRequest(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_make_contacts_request");
    proc.MakeContactsRequest();
}

void ContextLoadBlackboxSetdown(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_blackbox_setdown");
    proc.BlackboxSetdown();
}

void ContextLoadPrepareLaas(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_prepare_laas");
    proc.PrepareLaas();
}

void ContextLoadPrepareFlagsJson(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_flags_json");
    proc.PrepareFlagsJson();
}

void FakeContextLoad(NAppHost::IServiceContext& ctx, TLogContext logContext) {
    TContextLoadProcessor proc(ctx, std::move(logContext), "context_load_fake");
    proc.Fake();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
