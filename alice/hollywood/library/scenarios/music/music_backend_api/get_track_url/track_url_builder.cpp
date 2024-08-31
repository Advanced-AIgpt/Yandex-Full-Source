#include "track_url_builder.h"
#include "signature_token.h"

#include <library/cpp/string_utils/quote/quote.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TStringBuilder&& AddCgiParams(TStringBuilder&& url, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params) {
    if (params.size() == 0) {
        return std::move(url);
    }
    url << '?';
    TStringBuf sep;
    for (auto& p : params) {
        url << sep;
        AppendCgiEscaped(p.first, url);
        url << '=';
        AppendCgiEscaped(p.second, url);
        sep = "&";
    }
    return std::move(url);
}

} // namespace

TString BuildTrackUrl(const TStringBuf trackId, const TStringBuf from, const TStringBuf uid,
                      const TXmlRespParseResult& dlInfoXml) {
    auto token = CalculateSignatureToken(dlInfoXml);

    return AddCgiParams(
        TStringBuilder() << "https://" << dlInfoXml.Host << "/get-mp3/" << token << '/' << dlInfoXml.Ts
                         << dlInfoXml.Path,
        {
            {TStringBuf("track-id"), trackId},
            {TStringBuf("from"),     from},
            {TStringBuf("play"),     TStringBuf("false")},
            {TStringBuf("uid"),      uid}
        });
}

} // namespace NAlice::NHollywood::NMusic
