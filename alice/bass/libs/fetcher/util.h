#pragma once

#include "request.h"

#include <util/generic/strbuf.h>
#include <util/string/builder.h>
#include <util/string/split.h>

namespace NHttpFetcher {

inline constexpr TStringBuf HEADER_XSRCRWR_BASS_VALUE = "BASS";
inline constexpr TStringBuf HEADER_XSRCRWR_REQWIZARD_VALUE = "ReqWizard";

void AddHeaderToString(TStringBuf key, TStringBuf value, TString& headers);
void AddHeadersToString(const THttpHeaders& headers, TString& out);
bool IsLocalhost(TStringBuf host);

void AddCgiParametersToString(const TCgiParameters& cgi, TString& url);
TString UrlWithCgiParams(const NUri::TUri& uri, const TCgiParameters& cgi);

template <typename TOnNameValue>
void ParseSrcRwr(TStringBuf value, TOnNameValue&& onNameValue) {
    StringSplitter(value).Split(';').Consume([&onNameValue](TStringBuf part) {
        const TStringBuf name = part.NextTok('=');
        if (!name.empty() && !part.empty())
            onNameValue(name, part);
    });
}

template <typename TOnNameValue>
void ParseSrcRwrCgi(TStringBuf value, TOnNameValue&& onNameValue) {
    const TStringBuf name = value.NextTok(':');
    if (!name.empty() && !value.empty()) {
        onNameValue(name, value);
    }
}

} // namespace NHttpFetcher
