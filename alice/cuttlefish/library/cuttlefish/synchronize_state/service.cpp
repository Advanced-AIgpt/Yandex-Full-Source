#include "service.h"

#include "auth.h"
#include "flags_json.h"
#include "laas.h"
#include "quasar_iot.h"
#include "tvm.h"
#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/common/blackbox.h>
#include <alice/cuttlefish/library/cuttlefish/common/datasync.h>
#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/cuttlefish/converter/service.h>
#include <alice/cuttlefish/library/cuttlefish/stream_converter/megamind.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/cuttlefish/library/surface_mapper/mapper.h>

#include <alice/cuttlefish/library/utils/string_utils.h>

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/uniproxy/library/uaas_mapper/uaas_mapper.h>

#include <laas/lib/ip_properties/proto/ip_properties.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/utils/uuid.h>
#include <yweb/webdaemons/icookiedaemon/icookie_lib/process.h>

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>

#include <util/string/builder.h>
#include <util/string/util.h>


using namespace NAliceProtocol;

namespace NAlice::NCuttlefish::NAppHostServices::NSynchronizeState {

namespace {

template <typename MessageT>
inline MessageT Parse(const NAppHost::NService::TProtobufItem& it) {
    MessageT msg;
    it.Fill(&msg);
    return msg;
}

inline char GuessIpVersion(TStringBuf ipStr) {
    return ipStr.find(':') == TStringBuf::npos ? '4' : '6';
}

NAppHostHttp::THttpRequest CreateApiKeysRequest(const NAliceCuttlefishConfig::TConfig& cfg, TStringBuf clientKey, TStringBuf clientIp, TStringBuf serviceName)
{
    static constexpr TStringBuf JS_SERVICE_NAME = "jsapi";

    NAppHostHttp::THttpRequest req;
    req.SetMethod(NAppHostHttp::THttpRequest::Get);
    req.SetScheme(NAppHostHttp::THttpRequest::Http);

    const TString& token = (serviceName == JS_SERVICE_NAME) ? cfg.api_keys().js_token() : cfg.api_keys().mobile_token();

    req.SetPath(TStringBuilder() << "/check_key?"
        "service_token=" << token << "&"
        "key=" << clientKey << "&"
        "user_ip=" << clientIp << "&"
        "ip_v=" << GuessIpVersion(clientIp)
    );
    return req;
}

inline bool NeedBlackBoxRequest(const NAliceProtocol::TUserInfo& userInfo) {
    return userInfo.GetAuthTokenType() == NAliceProtocol::TUserInfo::OAUTH;
}

inline TMaybe<TString> GenerateICookieFromUuid(TString uuid) {
    RemoveAll(uuid, '-');
    return NIcookie::GenerateIcookieFromUuid(uuid);
}

const unsigned MIN_SPPECHKIT_VERSION_FOR_SYNCHRONIZE_STATE_RESPONSE = 4006000;  // 4.6.0

unsigned SpeechkitVersionAsNumber(TStringBuf ver)
{
    unsigned major, minor, micro;
    if (!TrySplitAndCast(ver, '.', major, minor, micro))
        return 0;
    if (major >= 1000 || minor >= 1000 || micro >= 1000)
        return 0;
    return major * 1000000 + minor * 1000 + micro;
}

}  // anonymous namespace


void TRequestContext::PreprocessDeviceInfo(const TSynchronizeStateEvent &event) {
    if (!event.HasDeviceInfo()) {
        return;
    }
    TDeviceInfo* const deviceInfo = SessionCtx.MutableDeviceInfo();
    Y_ENSURE(deviceInfo);
    *deviceInfo = event.GetDeviceInfo();
}

NAliceProtocol::TUserInfo* TRequestContext::PreprocessUserInfo(const TSynchronizeStateEvent &event) {
    NAliceProtocol::TUserInfo* userInfo = SessionCtx.MutableUserInfo();
    Y_ENSURE(userInfo);
    userInfo->MergeFrom(event.GetUserInfo());

    Y_ENSURE(userInfo->HasUuid());

    if (event.HasICookie()) {
        if (TMaybe<TString> decrypted = NIcookie::DecryptIcookie(event.GetICookie(), /* canThrow = */ false)) {
            userInfo->SetICookie(std::move(*decrypted));
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Set decrypted ICookie");
        } else {
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("Could not decrypted client's ICookie");
        }
    } else {
        if (auto maybeICookie = GenerateICookieFromUuid(userInfo->GetUuid())) {
            userInfo->SetICookie(std::move(*maybeICookie));
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Set ICookie generated from UUID");
        } else {
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("Could not generate ICookie from UUID");
        }
    }
    return userInfo;
}

void TRequestContext::PreprocessAppToken(const TSynchronizeStateEvent &event) {
    const TString& appToken = event.GetAppToken();
    if (!appToken) {
        LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("AppToken is absent");
        throw CreateEventExceptionEx("SYNCHRONIZE_STATE_PRE", "", "Invalid auth_token");
    }

    SessionCtx.SetAppToken(appToken);  // if ApiKeys rejects the key it'll be removed
    if (Find(Config.api_keys().whitelist().begin(), Config.api_keys().whitelist().end(), appToken) != Config.api_keys().whitelist().end()) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("AppToken is in whitelist");
    } else {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "AppToken is not in whitelist, add '" << ITEM_TYPE_APIKEYS_HTTP_REQUEST << "' item");
        ServiceCtx.AddProtobufItem(CreateApiKeysRequest(Config, appToken, SessionCtx.GetConnectionInfo().GetIpAddress(), event.GetServiceName()), ITEM_TYPE_APIKEYS_HTTP_REQUEST);
    }

    if (appToken == TStringBuf("51ae06cc-5c8f-48dc-93ae-7214517679e6"))  // the unique quasar's token
        SessionCtx.SetClientType(TSessionContext::CLIENT_TYPE_QUASAR);
}

void TRequestContext::PreprocessAppId(const TApplicationInfo& appInfo) {
    if (!appInfo.HasId()) {
        return;
    }

    const TString& appId = appInfo.GetId();
    SessionCtx.SetAppId(appId);
    SessionCtx.SetAppType(Config.app_types().mapper().Value(appId, "other_apps"));

    const NVoice::NSurfaceMapper::TSurfaceInfo& surfaceInfo = NVoice::NSurfaceMapper::GetMapperRef().Map({
        .AppInfo = {
            .AppId = appId,
        },
    });
    SessionCtx.SetSurface(surfaceInfo.GetSurface());
    SessionCtx.SetSurfaceType(static_cast<NAliceProtocol::TSessionContext::ESurfaceType>(surfaceInfo.GetType()));
}

void TRequestContext::PreprocessApplicationInfo(const TSynchronizeStateEvent& event) {
    if (!event.HasApplicationInfo()) {
        return;
    }

    auto& appInfo = event.GetApplicationInfo();
    PreprocessAppId(appInfo);
    if (appInfo.HasSpeechkitVersion()) {
        SessionCtx.SetSpeechkitVersion(appInfo.GetSpeechkitVersion());
    }
    if (appInfo.HasVersion()) {
        SessionCtx.SetAppVersion(appInfo.GetVersion());
    }
    if (appInfo.HasLang()) {
        SessionCtx.SetAppLang(appInfo.GetLang());
    }
}

void TRequestContext::SetDevicePlatform() {
    if (!SessionCtx.HasAppId() || !SessionCtx.GetDeviceInfo().HasPlatform()) {
        return;
    }
    const TString& appInfoJson = NVoice::GetUaasInfoJsonByClientInfo({
        .AppInfo = {
            .AppId = SessionCtx.GetAppId(),
        },
        .DeviceInfo = {
            .DeviceModel = SessionCtx.GetDeviceInfo().GetDeviceModel(),
            .Platform = SessionCtx.GetDeviceInfo().GetPlatform(),
            .DeviceModification = SessionCtx.GetDeviceInfo().GetDeviceModification(),
        }
    });
    SessionCtx.MutableExperiments()->MutableFlagsJsonData()->SetAppInfo(appInfoJson);
}

void TRequestContext::PreprocessFlagsDotJsonExperiments(const TSynchronizeStateEvent& event) {
    // test-ids from session context are useful when we make request to flags.json in context_load.

    for (const TString& testId : event.GetUaasTests()) {
        SessionCtx.MutableExperiments()->AddUaasTests(testId);
    }

    if (ConductingExperiment(event.GetExperiments(), NExpFlags::ONLY_100_PERCENT_FLAGS)) {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("Use only released flags from flags.json due to DISREGARD_UAAS exp flag");
        SessionCtx.MutableExperiments()->SetOnly100PercentFlagsForSession(true);
    } else {
        if (ConductingExperiment(event.GetExperiments(), NExpFlags::DISREGARD_UAAS)) {
            SessionCtx.MutableExperiments()->SetDisregardUaasForSession(true);
            return;
        }
    }
}

void TRequestContext::PreprocessWsMessage() {
    if (!ServiceCtx.HasProtobufItem(ITEM_TYPE_WS_MESSAGE)) {
        LogEvent(NEvClass::WarningMessage(TStringBuilder() << "Failed to enrich SpeechKitRequest, no " << ITEM_TYPE_WS_MESSAGE << " item present"));
        return;
    }

    TWsEvent wsEvent = ServiceCtx.GetOnlyProtobufItem<TWsEvent>(ITEM_TYPE_WS_MESSAGE);
    UpdateSpeechKitRequest(*SessionCtx.MutableRequestBase(), SessionCtx, wsEvent);
}

void TRequestContext::Preprocess()
{
    LogEvent(NEvClass::InfoMessage(TStringBuilder() << "PREPROCESS SessionId=" << SessionCtx.GetSessionId()));
    NAliceProtocol::TEvent e;
    if (!RawToProtobufImpl(ServiceCtx, LogContext, e)) {
        if (ServiceCtx.HasProtobufItem(ITEM_TYPE_SYNCRHONIZE_STATE_EVENT)) {
            e = ServiceCtx.GetOnlyProtobufItem<NAliceProtocol::TEvent>(ITEM_TYPE_SYNCRHONIZE_STATE_EVENT);
        } else {
            ServiceCtx.AddProtobufItem(
                CreateEventExceptionEx("SYNCHRONIZE_STATE_PRE", "", "Event SynchronizeState was not found while preprocessing sync_state"),
                ITEM_TYPE_DIRECTIVE
            );
            return;
        }
    }

    const TSynchronizeStateEvent& event = e.GetSyncState();
    LogEvent(NEvClass::InfoMessage(TStringBuilder() << "EVENT " << event));

    SessionCtx.SetInitialMessageId(e.GetHeader().GetMessageId());

    PreprocessApplicationInfo(event);

    PreprocessDeviceInfo(event);
    // USER OPTIONS
    if (event.HasUserOptions()) {
        TUserOptions* const userOptions = SessionCtx.MutableUserOptions();
        Y_ENSURE(userOptions);
        *userOptions = event.GetUserOptions();
    }

    NAliceProtocol::TUserInfo* userInfo = PreprocessUserInfo(event);

    PreprocessAppToken(event);

    HandleBlackboxAuthorization(*userInfo, SessionCtx.GetConnectionInfo());

    SetDevicePlatform();

    PreprocessFlagsDotJsonExperiments(event);  // here we put test-ids to sessionCtx

    // DATASYNC
    AddHttpRequestToDraft(ServiceCtx, TDatasyncClient::LoadPersonalSettingsRequest(), ITEM_TYPE_DATASYNC_HTTP_REQUEST);

    // AudioOptions
    if (event.HasAudioOptions()) {
        *SessionCtx.MutableAudioOptions() = event.GetAudioOptions();
    }

    // AudioOptions
    if (event.HasBiometryOptions()) {
        *SessionCtx.MutableBiometryOptions() = event.GetBiometryOptions();
    }

    // VoiceOptions
    if (event.HasVoiceOptions()) {
        *SessionCtx.MutableVoiceOptions() = event.GetVoiceOptions();
    }

    PreprocessWsMessage();

    // REQUEST BASE
    if (event.HasExperiments()) {
        auto& expProto = *SessionCtx.MutableRequestBase()->MutableRequest()->MutableExperiments();
        expProto.MergeFrom(event.GetExperiments());
    }
}


void TRequestContext::HandleBlackboxAuthorization(NAliceProtocol::TUserInfo& userInfo, const NAliceProtocol::TConnectionInfo& connInfo)
{

    if (userInfo.HasAuthToken()) {
        if (NeedBlackBoxRequest(userInfo)) {
            LogEvent(NEvClass::InfoMessage("Add BlackBox request"));
            ServiceCtx.AddProtobufItem(
                TBlackboxClient::GetUidForOAuth(userInfo.GetAuthToken(), "127.0.0.1"),
                ITEM_TYPE_BLACKBOX_HTTP_REQUEST
                );
        }
        return;
    }
    LogContext.LogEventInfoCombo<NEvClass::InfoMessage>("No user's auth token");


    TString sessionId;
    if (connInfo.HasCookie()) {
        sessionId = ExtractSessionIdFromCookie(connInfo.GetCookie());
    } else if (connInfo.HasXYambCookie()) {
        sessionId = ExtractSessionIdFromCookie(connInfo.GetXYambCookie());
    }

    if (!sessionId.empty()) {
        LogEvent(NEvClass::InfoMessage("Add BlackBox request"));
        ServiceCtx.AddProtobufItem(
            TBlackboxClient::GetUidForSessionId(sessionId, connInfo.GetIpAddress(), connInfo.GetOrigin()),
            ITEM_TYPE_BLACKBOX_HTTP_REQUEST
        );
    }

}


// ------------------------------------------------------------------------------------------------
void TRequestContext::Postprocess()
{
    LogEvent(NEvClass::InfoMessage(TStringBuilder() << "POSTPROCESS SessionID=" << SessionCtx.GetSessionId()));

    NAlice::NCuttlefish::TSourceMetrics metrics(SessionCtx, "synchronize_state_post");

    const auto items = ServiceCtx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = items.begin(); it != items.end(); ++it) {
        const TStringBuf type = it.GetType();
        LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Got " << it.GetTag() << "@" << type));

        if (type == ITEM_TYPE_BLACKBOX_HTTP_RESPONSE) {
            const NAppHostHttp::THttpResponse resp = Parse<NAppHostHttp::THttpResponse>(*it);
            LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Blackbox response: " << resp));
            if (!HandleBlackboxOAuthResponse(resp)) {
                if (SessionCtx.GetUserOptions().GetAcceptInvalidAuth()) {
                    auto directive = CreateInvalidAuth(SessionCtx.GetInitialMessageId());
                    ProtobufToRawImpl(ServiceCtx, directive);
                    ServiceCtx.AddProtobufItem(std::move(directive), ITEM_TYPE_DIRECTIVE);
                }
            }
            continue;
        }

        // NOTE: if ApiKeys doesn't respond auth_token is considered valid
        if (type == ITEM_TYPE_APIKEYS_HTTP_RESPONSE) {
            const NAppHostHttp::THttpResponse resp = Parse<NAppHostHttp::THttpResponse>(*it);
            LogEvent(NEvClass::InfoMessage(TStringBuilder() << "ApiKeys response: " << resp));
            if (resp.GetStatusCode() != 200) {
                throw CreateEventExceptionEx("SYNCHRONIZE_STATE_POST", "", "Invalid auth_token", SessionCtx.GetInitialMessageId());
            }
            continue;
        }

        if (type == ITEM_TYPE_DATASYNC_HTTP_RESPONSE) {
            const NAppHostHttp::THttpResponse resp = Parse<NAppHostHttp::THttpResponse>(*it);
            LogEvent(NEvClass::InfoMessage(TStringBuilder() << "DataSync response: " << resp));

            const auto settings = TDatasyncClient::ParsePersonalSetingsResponse(resp.GetContent());
            if (settings.DoNotUseUserLogs) {
                SessionCtx.MutableUserOptions()->SetDoNotUseLogs(*settings.DoNotUseUserLogs);
            }
            continue;
        }

        if (type == ITEM_TYPE_DIRECTIVE) {
            ProtobufToRawImpl(ServiceCtx, Parse<NAliceProtocol::TDirective>(*it));
            continue;
        }
    }

    const NAliceProtocol::TUserInfo& userInfo = SessionCtx.GetUserInfo();

    // check DoNotUseLogs
    TUserOptions& userOptions = *SessionCtx.MutableUserOptions();
    if (NeedBlackBoxRequest(userInfo)) {
        if (userInfo.HasPuid()) {  // BB succeeded
            if (!userOptions.HasDoNotUseLogs()) {  // ...but DataSync failed
                userOptions.SetDoNotUseLogs(true);
            }
        } else {  // BB failed
            if (!userOptions.HasDoNotUseLogs()) {
                userOptions.SetDoNotUseLogs(true);
            }
        }
    } else {  // there was no BB request
        if (!userOptions.HasDoNotUseLogs()) {
            userOptions.SetDoNotUseLogs(false);
        }
    }

    if (SessionCtx.HasSpeechkitVersion()) {
        if (SpeechkitVersionAsNumber(SessionCtx.GetSpeechkitVersion()) >= MIN_SPPECHKIT_VERSION_FOR_SYNCHRONIZE_STATE_RESPONSE) {
            auto directive = CreateSynchronizeStateResponse(
                SessionCtx.GetSessionId(),
                userInfo.GetGuid(),
                SessionCtx.GetInitialMessageId()
            );
            ProtobufToRawImpl(ServiceCtx, directive);
            ServiceCtx.AddProtobufItem(
                std::move(directive),
                ITEM_TYPE_DIRECTIVE
            );
        }
    }
}


bool TRequestContext::HandleBlackboxOAuthResponse(const NAppHostHttp::THttpResponse& resp)
{
    /** @NOTE: currently both HTTP and parsing errors are considered as "invalid authorization"
     * Probably we need a flag into TSessionContext if the authorization failed
    */

    NAliceProtocol::TUserInfo* userInfo = SessionCtx.MutableUserInfo();
    Y_ASSERT(userInfo);


    if (resp.GetStatusCode() != 200) {
        return false;
    }

    const auto parsed = TBlackboxClient::ParseResponse(resp.GetContent());
    if (!parsed.Valid) {
        return false;
    }

    userInfo->SetPuid(parsed.Uid);
    if (parsed.StaffLogin)
        userInfo->SetStaffLogin(parsed.StaffLogin);
    return true;
}


// ------------------------------------------------------------------------------------------------
void TRequestContext::BlackboxSetdown()
{
    TBlackboxClient::TOAuthResponse blackBoxResponse = {};
    try {
        const auto raw = ServiceCtx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(ITEM_TYPE_BLACKBOX_HTTP_RESPONSE);
        LogEvent(NEvClass::InfoMessage(TStringBuilder() << "BlackBox response: " << raw));

        if (raw.GetStatusCode() == 200) {
            blackBoxResponse = TBlackboxClient::ParseResponse(raw.GetContent());
            if (!blackBoxResponse.Valid) {
                LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("BlackBox rejected token");
            }
        } else {
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "BlackBox request failed with " << raw.GetStatusCode() << " code");
        }
    } catch (const std::exception& exc) {
        LogContext.LogEventErrorCombo<NEvClass::WarningMessage>("BlackBox didn't answer");  // or it was no request
    }

    const auto items = ServiceCtx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
    for (auto it = items.begin(); it != items.end(); ++it) {
        const TStringBuf type = it.GetType();
        LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Got '" << it.GetTag() << "@" << type << "' item"));

        if (type == ITEM_TYPE_HTTP_REQUEST_DRAFT) {
            NAppHostHttp::THttpRequest req = Parse<NAppHostHttp::THttpRequest>(*it);
            const TStringBuf draftType = GetHttpRequestDraftType(req);

            if (draftType == ITEM_TYPE_DATASYNC_HTTP_REQUEST) {
                if (blackBoxResponse.UserTicket && blackBoxResponse.Uid) {
                    AddHeader(req, "X-Ya-User-Ticket", blackBoxResponse.UserTicket);
                    AddHeader(req, "X-Uid", blackBoxResponse.Uid);
                    LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Request made from draft: " << req));
                } else {
                    LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>("Drop request to DataSync due to failed BlackBox");
                    continue;
                }
            }

            AddHttpRequestFromDraft(ServiceCtx, std::move(req));
            continue;
        }
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices::NSynchronizeState
