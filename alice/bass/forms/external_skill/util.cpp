#include "util.h"

#include <alice/bass/forms/context/context.h>

#include <contrib/libs/openssl/include/openssl/hmac.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/hex.h>
#include <util/string/util.h>

namespace NBASS {
namespace NExternalSkill {

TString HmacHex(TStringBuf secret, TStringBuf data, const EVP_MD* evpPtr) {
    char result[EVP_MAX_MD_SIZE];
    unsigned int resultSize = 0;

    if (!::HMAC(evpPtr, secret.data(), secret.size(),
                reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                reinterpret_cast<unsigned char*>(result), &resultSize))
    {
        return TString();
    }

    return HexEncode(result, resultSize);
}

TString HmacSha256Hex(TStringBuf secret, TStringBuf data) {
    return HmacHex(secret, data, EVP_sha256());
}

TString HmacMd5Hex(TStringBuf secret, TStringBuf data) {
    return HmacHex(secret, data, EVP_md5());
}

TString SignUrlForRedirector(TStringBuf secret, TStringBuf clientId, TStringBuf url) {
    // ATT: it is very important to keep this vector sorted by the first pair value
    // it is requirement for using redirector API
    const TVector<std::pair<TStringBuf, TStringBuf>> urlArgs = {
        { TStringBuf("client"), clientId },
        { TStringBuf("url"),    url },
    };

    TStringBuilder paramsString;
    bool firstTime = true;
    for(const auto& kv: urlArgs) {
        if (firstTime) {
            firstTime = false;
        } else {
            paramsString << '&';
        }
        paramsString << kv.first << '=' << kv.second;
    }
    paramsString << clientId;
    TString sign = HmacMd5Hex(secret, paramsString);
    sign.to_lower();
    return sign;
}

TString WrapUrlWithRedirector(const TContext& ctx, TStringBuf url) {
    const auto& redirectApi = ctx.GetConfig()->RedirectApi();

    TCgiParameters cgi;
    cgi.InsertUnescaped("url", url);
    cgi.InsertUnescaped("client", redirectApi.ClientId());
    cgi.InsertUnescaped("sign", SignUrlForRedirector(redirectApi.Key(), redirectApi.ClientId(), url));

    TStringBuilder fullUrl;

    fullUrl << redirectApi.Url();
    addIfNotLast(fullUrl, '?');
    fullUrl << cgi.Print();

    return fullUrl;
}

} // namespace NExternalSkill
} // namespace NBASS
