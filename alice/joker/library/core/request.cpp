#include "request.h"
#include "globalctx.h"

#include <alice/joker/library/status/status.h>

#include <library/cpp/openssl/crypto/sha.h>

#include <util/generic/serialized_enum.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>
#include <util/string/hex.h>

namespace NAlice::NJoker {
namespace {

constexpr TStringBuf JOKER_REQUEST_HEADER = "x-yandex-joker";

NUri::TUri ConstructUri(const TParsedHttpFull& http) {
    NUri::TUri uri;
    if (uri.ParseUri(http.Request, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible) != NUri::TUri::EParsed::ParsedOK) {
        ythrow yexception() << "invalid or unsupported uri: " << http.Request;
    }
    return uri;
}

THttpContext::THeaders ConstructHeaders(const THttpHeaders& headers, TMaybe<TString>* jokerHeader, TMaybe<TString>* viaProxy) {
    Y_ASSERT(jokerHeader);
    Y_ASSERT(viaProxy);

    THttpContext::THeaders parsedHeaders;
    parsedHeaders.reserve(headers.Count());

    for (const auto& h : headers) {
        if (h.Name() == TStringBuf("x-yandex-via-proxy")) {
            if (!viaProxy->Defined()) {
                viaProxy->ConstructInPlace(h.Value());
            } else {
                *viaProxy = h.Value();
            }
        } else if (h.Name() == JOKER_REQUEST_HEADER && !jokerHeader->Defined()) {
            jokerHeader->ConstructInPlace(h.Value());
        } else {
            parsedHeaders.emplace_back(h);
        }
    }

    Sort(parsedHeaders.begin(), parsedHeaders.end(),
         [](const THttpContext::THeader& lhs, const THttpContext::THeader& rhs) { return lhs.Name() < rhs.Name(); }
    );

    return parsedHeaders;
}

} // namespace

// IHttpContext ---------------------------------------------------------------
// static
TCgiParameters IHttpContext::ConstructCgi(const THeaders& headers, const NUri::TUri& uri, const TString& body, bool& isCgiReadFromBody) {
    TStringBuf cgiString;
    auto contentType = FindIf(headers.begin(), headers.end(), [](const auto& header) {
        return AsciiEqualsIgnoreCase(header.Name(), "content-type");
    });
    if (contentType != headers.end() && contentType->Value() == TStringBuf("application/x-www-form-urlencoded")) {
        isCgiReadFromBody = true;
        cgiString = body;
    } else {
        cgiString = uri.GetField(NUri::TField::EField::FieldQuery);
    }
    LOG(DEBUG) << "CGI requested string (" << (isCgiReadFromBody ? "BODY" : "URI") << "): " << cgiString << Endl;
    return TCgiParameters(cgiString);
}

// THttpContext ---------------------------------------------------------------
THttpContext::THttpContext(THttpInput& httpInput, THttpOutput& httpOutput)
    : IsCgiReadFromBody_{false}
    , Http_{httpInput.FirstLine()}
    , Headers_{ConstructHeaders(httpInput.Headers(), &JokerHeader_, &ViaProxy_)}
    , Body_{httpInput.ReadAll()}
    , Uri_{ConstructUri(Http_)}
    , FirstLine{httpInput.FirstLine()}
    , Cgi{ConstructCgi(Headers_, Uri_, Body_, IsCgiReadFromBody_)}
    , Output{httpOutput}
{
}

THttpContext::~THttpContext() {
    try {
        Output.Flush();
    } catch (...) {
        LOG(ERROR) << "Unable to flush network output: " << CurrentExceptionMessage() << Endl;
    }
}

TString THttpContext::CreateRequestHash(const TConfig& config) const {
    // sorted cgi, sorted headers, body, http method, uri (scheme+host+port+path)
    NOpenSsl::NSha256::TCalcer keyDigestCalcer;

    if (!config.SkipAllCGIs()) {
        const auto& skipCGIs = config.SkipCGIs();

        TVector<TString> cgiParams;
        cgiParams.reserve(Cgi.size());
        for (const auto& cgi : Cgi) {
            if (skipCGIs.empty() || !skipCGIs.contains(cgi.first)) {
                cgiParams.emplace_back(TStringBuilder() << cgi.first << cgi.second);
            }
        }
        Sort(cgiParams.begin(), cgiParams.end());
        keyDigestCalcer.Update(JoinSeq(TStringBuf(""), cgiParams));
    }

    if (!config.SkipAllHeaders()) {
        const auto& skipHeaders = config.SkipHeaders();

        TStringBuilder headers;
        for (const auto& h : Headers()) {
            if (skipHeaders.contains(to_lower(h.Name()))) {
                continue;
            }

            headers << h.ToString();
        }

        keyDigestCalcer.Update(headers);
    }

    if (!config.SkipBody() && !IsCgiReadFromBody_) {
        keyDigestCalcer.Update(Body());
    }

    keyDigestCalcer.Update(Http_.Method);
    keyDigestCalcer.Update(Uri_.PrintS(NUri::TField::EFlags::FlagAction));

    NOpenSsl::NSha256::TDigest digest = keyDigestCalcer.Final();
    return HexEncode(digest.data(), digest.size());
}

// static
void THttpContext::ErrorResponse(THttpOutput& output, const TError& error) {
    error.HttpResponse(output);
}

} // namespace NAlice::NJoker
