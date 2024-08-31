#include "metadata.h"

#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/string/cast.h>
#include <alice/gproxy/library/events/gproxy.ev.pb.h>


namespace NGProxy {

#define Y_SET_ONLY_ITEM(name_, field_) \
{ \
    if (name == name_) { \
        to.Set ## field_(value); \
    } \
}

const TString OAUTH_PREFIX = "OAuth ";

bool FillMetadata(const google::protobuf::RepeatedPtrField<NAppHostHttp::THeader>& from, TMetadata& to, TMaybe<NAlice::NCuttlefish::TLogContext> logContext) {
    bool internalRequest = false;

    for(const auto& header: from) {
        const auto& originalName = header.GetName();
        TString name = originalName;
        name.to_lower();

        const auto& value = header.GetValue();
        if (name == "x-yandex-internal-request" && value == "1") {
            internalRequest = true;
            to.SetInternalRequest(true);
        }

        Y_SET_ONLY_ITEM("x-ya-session-id",  SessionId);
        Y_SET_ONLY_ITEM("x-ya-request-id",  RequestId);
        Y_SET_ONLY_ITEM("x-ya-real-ipaddr", IpAddr);
        Y_SET_ONLY_ITEM("x-ya-device-id", DeviceId);
        Y_SET_ONLY_ITEM("x-ya-uuid", Uuid);
        Y_SET_ONLY_ITEM("x-ya-firmware", Firmware);
        Y_SET_ONLY_ITEM("x-ya-language", Language);
        Y_SET_ONLY_ITEM("x-ya-app-type", AppType);
        Y_SET_ONLY_ITEM("x-ya-app-id", AppId);
        Y_SET_ONLY_ITEM("x-ya-rtlog-token", RtLogToken);
        Y_SET_ONLY_ITEM("x-ya-application", Application);
        Y_SET_ONLY_ITEM("x-ya-tags", Tags);
        Y_SET_ONLY_ITEM("x-ya-location", Location);
        Y_SET_ONLY_ITEM("x-ya-oauthtoken", OAuthToken);
        if (internalRequest) {
            Y_SET_ONLY_ITEM("x-ya-experiments", Experiments);
        }
        Y_SET_ONLY_ITEM("x-real-ip", IpAddr);
        Y_SET_ONLY_ITEM("x-ya-user-lang", UserLang);
        Y_SET_ONLY_ITEM("x-ya-user-ticket", UserTicket);
        Y_SET_ONLY_ITEM("x-ya-user-agent", UserAgent);
        Y_SET_ONLY_ITEM("user-agent", UserAgent);
        if (name == "authorization" && value.StartsWith(OAUTH_PREFIX)) {
            to.SetOAuthToken(value.substr(OAUTH_PREFIX.size()));
        }

        if (name == "x-ya-extra-logging" && value == "1") {
            to.SetExtraLogging(true);
        }

        if (name == "x-ya-no-oauth-token") {
            if (value == "true") {
                to.SetNoOAuthToken(true);
            } else {
                to.SetNoOAuthToken(false);
            }
        }

        if (name == "x-ya-echo-request") {
            if (value == "true") {
                to.SetEchoRequest(true);
            } else {
                to.SetEchoRequest(false);
            }
        } else {
            to.SetEchoRequest(false);
        }

        // X-Ya-SupportedFeatures
        if (name == "x-ya-supported-features") {
            to.AddSupportedFeatures(value);
        }
        if (name == "x-ya-extra-response") {
            to.AddExtraResponse(value);
        }

        if (name == "x-ya-random-seed" && internalRequest) {
            ui64 seed = FromString<ui64>(value);
            to.SetRandomSeed(seed);
        }

        if (name == "x-ya-server-time-ms" && internalRequest) {
            const ui64 serverTimeMs = FromString<ui64>(value);
            to.SetServerTimeMs(serverTimeMs);
        } else {
            to.SetServerTimeMs(TInstant::Now().MilliSeconds());
        }
    }

    if (logContext.Defined()) {
        for (const auto& header : from) {
            TString key = header.GetName();
            if (LoggingIsAllowedForHeader(key)) {
                logContext.Get()->LogEventInfoCombo<NEvClass::GrpcHeader>(
                    std::move(key),
                    header.GetValue()
                );
            }
        }
    }

    return true;
}

#undef Y_SET_ONLY_ITEM


void FillRewrites(
    NJson::TJsonValue::TMapType* map,
    const TClientMetadata& from,
    grpc::string_ref rewriteMetaKey
) {
    Y_ASSERT(map != nullptr);

    auto [srcrwrIter, srcrwrEndIter]  = from.equal_range(rewriteMetaKey);
    for (; srcrwrIter != srcrwrEndIter; ++srcrwrIter) {
        const grpc::string_ref& target = srcrwrIter->second;
        TStringBuf src, rwr;
        if (TStringBuf(target.data(), target.size()).TrySplit(':', src, rwr) && src && rwr) {
            (*map)[ToString(src)] = ToString(rwr);
        }
    }
}

NJson::TJsonValue CreateAppHostParams(
    const TClientMetadata& from,
    bool allowSrcrwr,
    bool allowDumpReqResp,
    TString rtLogToken
) {
    NJson::TJsonValue params;

    {
        NJson::TJsonValue::TMapType& map = params["srcrwr"].SetType(NJson::JSON_MAP).GetMapSafe();

        map["SELF__VOICE"] = "VOICE__CUTTLEFISH";

        if (allowSrcrwr) {
            FillRewrites(&map, from, "x-srcrwr");
        }
    }

    if (allowSrcrwr) {
        NJson::TJsonValue::TMapType& map = params["graphrwr"].SetType(NJson::JSON_MAP).GetMapSafe();
        FillRewrites(&map, from, "x-graphrwr");
    }

    if (rtLogToken) {
        params["reqid"] = std::move(rtLogToken);
    }

    {
        auto it = from.find("x-json-dump");
        if (it != from.end() && it->second == "true") {
            params["json_dump_requests"]["*"]["filters"].SetType(NJson::JSON_ARRAY);
            params["json_dump_responses"]["*"]["filters"].SetType(NJson::JSON_ARRAY);
        }
    }

    if (allowDumpReqResp) {
        auto itReq = from.find("x-dump-source-requests");
        if (itReq != from.end() && itReq->second == "1") {
            params["dump_source_requests"] = TString("1");
        }
        auto itResp = from.find("x-dump-source-responses");
        if (itResp != from.end() && itResp->second == "1") {
            params["dump_source_responses"] = TString("1");
        }
    }

    return params;
}


bool LoggingIsAllowedForHeader(TString header) {
    static TSet<TString> whitelist {
        "x-srcrwr",
        "x-graphrwr",
        "x-real-ip",
        "x-ya-session-id",
        "x-ya-request-id",
        "x-ya-device-id",
        "x-ya-uuid",
        "x-ya-firmware",
        "x-ya-language",
        "x-ya-app-type",
        "x-ya-app-id",
        "x-ya-rtlog-token",
        "x-ya-application",
        "x-ya-tags",
        "x-ya-location",
        "x-ya-supported-features",
        "x-ya-user-agent",
        "authorization",
        "user-agent",
        "accept-language"
    };
    header.to_lower();
    return whitelist.contains(header);
}

}   // namespace NGProxy
