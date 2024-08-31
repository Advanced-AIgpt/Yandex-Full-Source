#include "util.h"

#include <alice/library/network/common.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/string/subst.h>

namespace NHttpFetcher {

using namespace NAlice;

void AddHeaderToString(TStringBuf key, TStringBuf value, TString& headers) {
    headers.reserve(headers.size() + CRLF.size() + key.size() + value.size() + 3);
    if (!headers.empty()) {
        headers.append(CRLF);
    }
    headers.append(key);
    headers.append(": ");
    headers.append(value);
}

void AddHeadersToString(const THttpHeaders& headers, TString& out) {
    Y_ASSERT(!headers.Empty());

    for (const auto& h : headers) {
        AddHeaderToString(h.Name(), h.Value(), out);
    }
}

void AddCgiParametersToString(const TCgiParameters& cgi, TString& url) {
    if (cgi) {
        bool empty = true;
        for (const auto& it : cgi) {
            if (empty) {
                url.append('?');
                empty = false;
            } else {
                url.append('&');
            }
            url.append(CGIEscapeRet(it.first));
            url.append('=');
            if (it.first == TStringBuf("rearr")) {
                // some kind of magic for 'rearr' parameters
                TString val = CGIEscapeRet(it.second);
                SubstGlobal(val, "+", "%20");
                url.append(val);
            } else {
                url.append(CGIEscapeRet(it.second));
            }
        }
    }
}

TString UrlWithCgiParams(const NUri::TUri& uri, const TCgiParameters& cgi) {
    TString url = uri.PrintS();
    if (!cgi.empty()) {
        AddCgiParametersToString(cgi, url);
    }
    return url;
}

bool IsLocalhost(TStringBuf host) {
    return host == TStringBuf("localhost") || host == TStringBuf("127.0.0.1") || host == TStringBuf("[::1]");
}

} // namespace NHttpFetcher
