#include "common.h"

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/string/strip.h>

namespace NAlice {
namespace NNetwork {
NUri::TUri ParseUri(TStringBuf url) {
    NUri::TUri uri;
    if (!TryParseUri(url, uri)) {
        ythrow TInvalidUrlException(TString{url});
    }
    return uri;
}

bool TryParseUri(TStringBuf url, NUri::TUri& uri) {
    const auto result = uri.Parse(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);
    return result == NUri::TState::EParsed::ParsedOK;
}

bool TryParseUri(TStringBuf url, NUri::TUri& uri, TCgiParameters& cgi) {
    TStringBuf cgiStr = url.SplitOff('?');
    if (!cgiStr.Empty()) {
        cgi.ScanAdd(cgiStr);
    }
    const auto result = uri.ParseUri(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);
    return result == NUri::TState::EParsed::ParsedOK;
}

NUri::TUri AppendToUri(NUri::TUri uri, TStringBuf appendPath) {
    AppendToUriInPlace(uri, appendPath);
    return uri;
}

void AppendToUriInPlace(NUri::TUri& uri, TStringBuf appendPath) {
    const TString path =
        TFsPath(uri.GetField(NUri::TField::FieldPath)) / StripStringLeft(appendPath, EqualsStripAdapter('/'));
    NUri::TUriUpdate(uri).Set(NUri::TField::FieldPath, path);
    uri = ParseUri(uri.PrintS());
}

TMaybe<HttpCodes> ToHttpCodes(int code) {
    if (code < 0)
        return {};
    if (IsHttpCode(code))
        return static_cast<HttpCodes>(code);

    // Effectively clears last two decimal digits, for example,
    // transforms 599 to 500.
    code = (code / 100) * 100;
    if (IsHttpCode(code))
        return static_cast<HttpCodes>(code);

    return {};
}

TString GetBlurValue(const TString& uuid, const ui64 seed, const ui32 blurRatio) {
    TString value{};
    if (blurRatio > 1) {
        value = ToString((THash<TString>{}(uuid) + seed) % blurRatio);
    }
    return value;
}

ui64 GetVinsBalancerHint(const TString& utterance, const TString& uuid, const ui64 seed, const ui32 blurRatio) {
    return CityHash64WithSeed(utterance, CityHash64(GetBlurValue(uuid, seed, blurRatio)));
}

} // namespace NNetwork
} // namespace NAlice
