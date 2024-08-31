#include "fake_fetcher.h"

#include <alice/bass/libs/fetcher/util.h>

#include <util/string/builder.h>

#include <algorithm>

namespace NAlice::NTestingHelpers {

// TFakeRequest ----------------------------------------------------------------
TVector<TString> TFakeRequest::GetHeaders() const {
    TVector<TString> result(Headers.size());
    std::transform(Headers.begin(), Headers.end(), result.begin(),
                   [](const auto& header) { return TString::Join(header.Name, ": ", header.Value); });
    return result;
}

TString TFakeRequest::Url() const {
    TStringBuilder uri;
    uri << "http://fake_fetcher.su";
    NHttpFetcher::AddCgiParametersToString(Cgi, uri);
    return uri;
}

// TFakeRequestBuilder ---------------------------------------------------------
TString TFakeRequestBuilder::DebugString() const {
    TStringBuilder data;

    data << Method << " ?" << Cgi.Print() << Endl;
    if (ContentType.Defined()) {
        data << "<content-type>: " << *ContentType << Endl;
    }
    for (const auto& h : Headers) {
        data << h.Name() << ": " << h.Value() << Endl;
    }
    if (Body.Defined()) {
        data << Endl;
        data << *Body;
    }

    return data;
}

} // namespace NAlice::NTestingHelpers
