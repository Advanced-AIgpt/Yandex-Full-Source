#include "signature_token.h"

#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/system/env.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const inline TString SECRET_ENV_KEY = "MUSICKIT_SECRET_KEY";
const inline TString HLS_SECRET_ENV_KEY = "MUSICKIT_HLS_SECRET_KEY";

} // namespace

namespace NImpl {

TString CalculateSignatureToken(const TXmlRespParseResult& result, const TStringBuf secret) {
    MD5 md5;
    md5.Update(secret);
    auto pathPart = TStringBuf(result.Path).substr(1);
    md5.Update(pathPart);
    md5.Update(result.Signature);

    char token[33] = {0,};
    md5.End(token);
    return TString{token, 32};
}

TString CalculateHlsSignatureToken(const TStringBuf trackId, TInstant ts, const TStringBuf secret) {
    unsigned char sign[EVP_MAX_MD_SIZE];
    unsigned int signLen;
    TStringBuilder data;
    data << trackId << ts.Seconds();
    HMAC(EVP_sha256(), secret.data(), secret.size(), reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         sign, &signLen);
    return Base64Encode(TStringBuf(reinterpret_cast<const char*>(sign), signLen));
}

}

TString CalculateSignatureToken(const TXmlRespParseResult& result) {
    static const auto secret = GetEnv(SECRET_ENV_KEY);
    return NImpl::CalculateSignatureToken(result, secret);
}

TString CalculateHlsSignatureToken(const TStringBuf trackId, TInstant ts) {
    static const auto secret = GetEnv(HLS_SECRET_ENV_KEY);
    Y_ENSURE(!secret.empty());
    return NImpl::CalculateHlsSignatureToken(trackId, ts, secret);
}

} // namespace NAlice::NHollywood::NMusic
