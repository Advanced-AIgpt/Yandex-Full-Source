#include "auth.h"


namespace NAlice::NCuttlefish::NAppHostServices {


EAuthTokenType GetOAuthTokenType(TStringBuf token) {
    Y_ASSERT(token);
    if (token.StartsWith(TConstants::OAUTHTEAM_TOKEN_PREFIX))
        return EAuthTokenType::OAuthTeam;
    if (token.StartsWith(TConstants::YAMBAUTH_TOKEN_PREFIX))
        return EAuthTokenType::YambAuth;
    return EAuthTokenType::OAuth;
}

TStringBuf StripOAuthToken(TStringBuf token) {
    if (token.StartsWith(TConstants::OAUTH_TOKEN_PREFIX))
        return token.Tail(TConstants::OAUTH_TOKEN_PREFIX.size());
    return token;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices