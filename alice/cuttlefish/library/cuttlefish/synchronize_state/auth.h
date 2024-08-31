#pragma once

#include "utils.h"

#include <alice/cuttlefish/library/cuttlefish/config/config.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/json/json_reader.h>

#include <util/string/builder.h>
#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {

struct TConstants {
    static constexpr TStringBuf OAUTHTEAM_TOKEN_PREFIX = "oauthteam";
    static constexpr TStringBuf YAMBAUTH_TOKEN_PREFIX = "yambauth";
    static constexpr TStringBuf OAUTH_TOKEN_PREFIX = "oauth";
};

enum class EAuthTokenType {
    Invalid,
    OAuthTeam,
    YambAuth,
    OAuth
};

EAuthTokenType GetOAuthTokenType(TStringBuf token);

TStringBuf StripOAuthToken(TStringBuf token);

}  // namespace NAlice::NCuttlefish::NAppHostServices

