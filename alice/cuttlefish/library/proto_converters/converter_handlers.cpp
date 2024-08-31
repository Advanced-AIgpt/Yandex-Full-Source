#include "converter_handlers.h"
#include <util/string/strip.h>
#include <util/string/ascii.h>


namespace NAlice::NCuttlefish::NProtoConverters {

namespace {

inline bool StartsWithCaseInsensitive(const TStringBuf a, const TStringBuf b) {
    return AsciiCompareIgnoreCase(a.Head(b.size()), b) == 0;
}

inline bool CheckAndSetToken(NAliceProtocol::TUserInfo& dst, const TStringBuf token, const TStringBuf prefix) {
    if (!StartsWithCaseInsensitive(token, prefix))
        return false;
    const TString stripped = Strip(TString(token.Tail(prefix.size())));
    dst.SetAuthToken(stripped);
    return true;
}

}  // anonymous namespace

void TAuthTokenHandler::Parse(const NJson::TJsonValue& src, NAliceProtocol::TUserInfo& dst)
{
    const TStringBuf token = src.GetStringSafe();
    if (!token)
        return;

    if (CheckAndSetToken(dst, token, OAUTH_TEAM_PREFIX)) {
        dst.SetAuthTokenType(NAliceProtocol::TUserInfo::OAUTH_TEAM);
        return;
    }
    if (CheckAndSetToken(dst, token, YAMB_AUTH_PREFIX)) {
        dst.SetAuthTokenType(NAliceProtocol::TUserInfo::YAMB_AUTH);
        return;
    }

    // this is default token type
    if (!CheckAndSetToken(dst, token, OAUTH_PREFIX))
        dst.SetAuthToken(TString(token));
    dst.SetAuthTokenType(NAliceProtocol::TUserInfo::OAUTH);
}


}  // NAlice::NCuttlefish::NProtoConverters