#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <array>

namespace NAlice::NNetwork {

inline const TString HEADER_CONTENT_TYPE{"Content-Type"};
inline const TString HEADER_CONTENT_LENGTH{"content-length"};
inline const TString HEADER_ACCEPT_ENCODING{"accept-encoding"};
inline constexpr TStringBuf HEADER_ACCEPT = "Accept";
inline constexpr TStringBuf HEADER_AUTHORIZATION = "authorization";
inline constexpr TStringBuf HEADER_COOKIE = "Cookie";
inline constexpr TStringBuf HEADER_HOST = "Host";
inline constexpr TStringBuf HEADER_LOCATION = "Location";
inline constexpr TStringBuf HEADER_SET_COOKIE = "Set-Cookie";
inline constexpr TStringBuf HEADER_USER_AGENT = "User-Agent";
inline constexpr TStringBuf HEADER_X_ALICE_APPID = "X-Alice-AppId";
inline constexpr TStringBuf HEADER_X_ALICE_CLIENT_REQID = "x-alice-client-reqid";
inline constexpr TStringBuf HEADER_X_APPHOST_REQUEST_REQID = "X-AppHost-Request-Reqid";
inline constexpr TStringBuf HEADER_X_APPHOST_REQUEST_RUID = "X-AppHost-Request-Ruid";
inline constexpr TStringBuf HEADER_X_BALANCER_DC_HINT = "x-yandex-balancing-hint";
inline constexpr TStringBuf HEADER_X_ALICE_INTERNAL_REQUEST = "x-yandex-alice-internal-request";
inline constexpr TStringBuf HEADER_X_DEVICE_AUDIO_CODECS = "X-Device-Audio-Codecs";
inline constexpr TStringBuf HEADER_X_DEVICE_ID = "X-Device-ID";
inline constexpr TStringBuf HEADER_X_FORWARDED_FOR = "x-forwarded-for";
inline constexpr TStringBuf HEADER_X_FORWARDED_FOR_Y = "x-forwarded-for-y";
inline constexpr TStringBuf HEADER_X_HOST = "X-Host";
inline constexpr TStringBuf HEADER_X_MARKET_REQ_ID = "x-market-req-id";
inline constexpr TStringBuf HEADER_X_OAUTH_TOKEN = "X-OAuth-Token";
inline constexpr TStringBuf HEADER_X_REAL_IP = "x-real-ip";
inline constexpr TStringBuf HEADER_X_REQUEST_ID = "X-Request-Id";
inline constexpr TStringBuf HEADER_X_REQ_ID = "X-Req-Id";
inline constexpr TStringBuf HEADER_X_RTLOG_TOKEN = "x-rtlog-token";
inline constexpr TStringBuf HEADER_X_SRCRWR = "x-srcrwr";
inline constexpr TStringBuf HEADER_X_UID = "X-Uid";
inline constexpr TStringBuf HEADER_X_YANDEX_ALICE_META_INFO = "X-Yandex-Alice-Meta-Info";
inline constexpr TStringBuf HEADER_X_YANDEX_APP_INFO = "X-Yandex-AppInfo";
inline constexpr TStringBuf HEADER_X_YANDEX_FAKE_TIME = "x-yandex-fake-time";
inline constexpr TStringBuf HEADER_X_YANDEX_ICOOKIE = "x-yandex-icookie";
inline constexpr TStringBuf HEADER_X_YANDEX_INTERNAL_FLAGS = "X-Yandex-Internal-Flags";
inline constexpr TStringBuf HEADER_X_YANDEX_INTERNAL_REQUEST = "X-Yandex-Internal-Request";
inline constexpr TStringBuf HEADER_X_YANDEX_MUSIC_CLIENT = "X-Yandex-Music-Client";
inline constexpr TStringBuf HEADER_X_YANDEX_MUSIC_DEVICE = "X-Yandex-Music-Device";
inline constexpr TStringBuf HEADER_X_YANDEX_PROXY_HEADER_NAME_BEGIN = "x-yandex-proxy-header-";
inline constexpr TStringBuf HEADER_X_YANDEX_REPORT_ALICE_HASH_ID = "X-Yandex-Report-Alice-Request-Hash-Id";
inline constexpr TStringBuf HEADER_X_YANDEX_REPORT_ALICE_REQUEST_INIT = "X-Yandex-Report-Alice-Request-Init";
inline constexpr TStringBuf HEADER_X_YANDEX_REQ_ID = "X-Yandex-Req-Id";
inline constexpr TStringBuf HEADER_X_YANDEX_VIA_PROXY = "x-yandex-via-proxy";
inline constexpr TStringBuf HEADER_X_YANDEX_VIA_PROXY_SKIP = "x-yandex-via-proxy-skip";
inline constexpr TStringBuf HEADER_X_YA_SERVICE_TICKET = "X-Ya-Service-Ticket";
inline constexpr TStringBuf HEADER_X_YA_USER_TICKET = "X-Ya-User-Ticket";
inline constexpr TStringBuf HEADER_X_VINS_REQUEST_HINT = "X-Vins-Request-Hint";

inline constexpr std::array<TStringBuf, 3> HEADERS_FOR_CLIENT_IP = {{
    HEADER_X_FORWARDED_FOR,
    HEADER_X_FORWARDED_FOR_Y,
    HEADER_X_REAL_IP
}};

} // namespace NAlice::NNetwork
