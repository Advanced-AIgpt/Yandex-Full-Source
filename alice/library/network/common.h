#pragma once

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/uri/uri.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NAlice {

inline constexpr TStringBuf CRLF = "\r\n";

namespace NContentTypes {
inline constexpr TStringBuf APPLICATION_JSON = "application/json";
inline constexpr TStringBuf APPLICATION_JSON_UTF_8 = "application/json; charset=utf-8";
inline constexpr TStringBuf APPLICATION_PROTOBUF = "application/protobuf";
inline constexpr TStringBuf TEXT_PLAIN = "text/plain";
inline constexpr TStringBuf TEXT_PLAIN_UTF_8 = "text/plain; charset=utf-8";
inline constexpr TStringBuf TEXT_PROTOBUF = "text/protobuf";
} // namespace NContentTypes

namespace NHttpMethods {
inline constexpr TStringBuf DELETE = "DELETE";
inline constexpr TStringBuf GET = "GET";
inline constexpr TStringBuf POST = "POST";
inline constexpr TStringBuf PUT = "PUT";
} // namespace NHttpMethods

namespace NNetwork {

inline constexpr TStringBuf SRCRWR = "srcrwr";

class TInvalidUrlException : public yexception {
public:
    TInvalidUrlException(const TString& url)
        : Url(url) {
        (*this) << "Invalid URL [" << Url << ']';
    }

    const TString& GetUrl() const {
        return Url;
    }

private:
    const TString Url;
};

// *NOTE* Throws TInvalidUrlException in case of failure.
NUri::TUri ParseUri(TStringBuf url);
// Same as ParseUri() but doesn't throw an exception.
bool TryParseUri(TStringBuf url, NUri::TUri& uri);
bool TryParseUri(TStringBuf url, NUri::TUri& uri, TCgiParameters& cgi);

// *NOTE* Throws TInvalidUrlException in case of failure.
NUri::TUri AppendToUri(NUri::TUri uri, TStringBuf appendPath);
void AppendToUriInPlace(NUri::TUri& uri, TStringBuf appendPath);

TMaybe<HttpCodes> ToHttpCodes(int code);

ui64 GetVinsBalancerHint(const TString& utterance, const TString& uuid, const ui64 seed, const ui32 blurRatio);

} // namespace NNetwork
} // namespace NAlice
