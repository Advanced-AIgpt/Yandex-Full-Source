#include "metadata.h"
#include "test_devices.h"

#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/string/cast.h>
#include <alice/gproxy/library/events/gproxy.ev.pb.h>


namespace NGProxy {

#define Y_SET_ONLY_ITEM(name, field) \
{ \
    auto it = from.find(name); \
    if (it != from.end()) { \
        to.Set ## field(TString(it->second.data(), it->second.data() + it->second.size())); \
    } \
}

const TString OAUTH_PREFIX = "OAuth ";

bool FillMetadata(const TClientMetadata& originalFrom, TMetadata& to, TMaybe<NAlice::NCuttlefish::TLogContext> logContext) {

    std::multimap<TString, grpc::string_ref> from;
    for (auto[key, value] : originalFrom) {
        TString loweredKey = key.data();
        loweredKey.to_lower();
        from.emplace(std::move(loweredKey), value);
    }

    bool internalRequest = false;
        {
        auto it = from.find("x-yandex-internal-request");
        if (it != from.end() && TStringBuf(it->second.data(), it->second.size()) == "1") {
            to.SetInternalRequest(true);
            internalRequest = true;
        }
    }

    Y_SET_ONLY_ITEM("x-ya-session-id",  SessionId);
    Y_SET_ONLY_ITEM("x-ya-request-id",  RequestId);
    Y_SET_ONLY_ITEM("x-ya-real-ipaddr", IpAddr);
    Y_SET_ONLY_ITEM("x-ya-device-id", DeviceId);
    Y_SET_ONLY_ITEM("x-ya-uuid", Uuid);
    Y_SET_ONLY_ITEM("x-ya-firmware", Firmware);
    Y_SET_ONLY_ITEM("x-ya-language", Language);
    Y_SET_ONLY_ITEM("accept-language", Language);
    Y_SET_ONLY_ITEM("x-ya-app-type", AppType);
    Y_SET_ONLY_ITEM("x-ya-app-id", AppId);
    Y_SET_ONLY_ITEM("x-ya-rtlog-token", RtLogToken);
    Y_SET_ONLY_ITEM("x-ya-application", Application);
    Y_SET_ONLY_ITEM("x-ya-tags", Tags);
    Y_SET_ONLY_ITEM("x-ya-location", Location);
    Y_SET_ONLY_ITEM("x-ya-oauthtoken", OAuthToken);
    if (internalRequest || IsTestDevice(to.GetDeviceId())) {
        Y_SET_ONLY_ITEM("x-ya-experiments", Experiments);
    }
    Y_SET_ONLY_ITEM("x-real-ip", IpAddr);
    Y_SET_ONLY_ITEM("x-ya-user-lang", UserLang);
    Y_SET_ONLY_ITEM("x-ya-user-ticket", UserTicket);
    Y_SET_ONLY_ITEM("x-ya-user-agent", UserAgent);
    Y_SET_ONLY_ITEM("user-agent", UserAgent);

    {
        auto it = from.find("authorization");
        if (it != from.end()) {
            TString header = TString(it->second.data(), it->second.data() + it->second.size());
            if (header.StartsWith(OAUTH_PREFIX)) {
                to.SetOAuthToken(header.substr(OAUTH_PREFIX.size()));
            }
        }
    }

    {
        auto it = from.find("x-ya-extra-logging");
        if (it != from.end() && TStringBuf(it->second.data(), it->second.size()) == "1") {
            to.SetExtraLogging(true);
        }
    }

    {
        auto it = from.find("x-ya-no-oauth-token");
        if (it != from.end() && it->first == "x-ya-no-oauth-token") {
            auto value = TString(it->second.data(), it->second.data() + it->second.size());
            if (value == "true") {
                to.SetNoOAuthToken(true);
            } else {
                to.SetNoOAuthToken(false);
            }
        }
    }

    {
        auto it = from.find("x-ya-echo-request");
        if (it != from.end() && it->first == "x-ya-echo-request") {
            auto value = TString(it->second.data(), it->second.data() + it->second.size());
            if (value == "true") {
                to.SetEchoRequest(true);
            } else {
                to.SetEchoRequest(false);
            }
        } else {
            to.SetEchoRequest(false);
        }
    }

    // X-Ya-SupportedFeatures
    {
        auto it = from.find("x-ya-supported-features");
        while (it != from.end() && it->first == "x-ya-supported-features") {
            to.AddSupportedFeatures(TString(it->second.data(), it->second.data() + it->second.size()));
            ++it;
        }
    }

    // X-Ya-UnsupportedFeatures
    {
        auto it = from.find("x-ya-unsupported-features");
        while (it != from.end() && it->first == "x-ya-unsupported-features") {
            to.AddUnsupportedFeatures(TString(it->second.data(), it->second.data() + it->second.size()));
            ++it;
        }
    }

    {
        auto it = from.find("x-ya-extra-response");
        while (it != from.end() && it->first == "x-ya-extra-response") {
            to.AddExtraResponse(TString(it->second.data(), it->second.data() + it->second.size()));
            ++it;
        }
    }

    if (internalRequest) {
        auto it = from.find("x-ya-random-seed");
        if (it != from.end()) {
            TStringBuf seedStr(it->second.data(), it->second.size());
            ui64 seed = FromString<ui64>(seedStr);
            to.SetRandomSeed(seed);
        }
    }

    {
        auto it = from.find("x-ya-server-time-ms");
        if (it != from.end() && internalRequest) {
            const TStringBuf serverTimeMsStr(it->second.data(), it->second.size());
            const ui64 serverTimeMs = FromString<ui64>(serverTimeMsStr);
            to.SetServerTimeMs(serverTimeMs);
        } else {
            to.SetServerTimeMs(TInstant::Now().MilliSeconds());
        }
    }

    if (logContext.Defined()) {
        for (const auto& [keyRef, valueRef] : from) {
            TString key(keyRef.data(), keyRef.size());
            if (LoggingIsAllowedForHeader(key)) {
                logContext.Get()->LogEventInfoCombo<NEvClass::GrpcHeader>(
                    std::move(key),
                    TString(valueRef.data(), valueRef.size())
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
        "user-agent",
        "authorization",
        "accept-language"
    };
    header.to_lower();
    return whitelist.contains(header);
}

}   // namespace NGProxy
