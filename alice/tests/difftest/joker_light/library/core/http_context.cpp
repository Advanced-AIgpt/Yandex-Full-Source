#include "http_context.h"
#include "context.h"

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/openssl/crypto/sha.h>

#include <util/generic/serialized_enum.h>
#include <util/string/hex.h>
#include <util/string/join.h>

namespace NAlice::NJokerLight {
namespace {

constexpr TStringBuf JOKER_REQUEST_HEADER = "X-Yandex-Joker";
constexpr TStringBuf JOKER_HOST_HEADER = "X-Host";
constexpr TStringBuf JOKER_PROXY_HEADER = "X-Yandex-Via-Proxy";
constexpr TStringBuf JOKER_PROXY_FAKE_TIME = "X-Yandex-Fake-Time";
constexpr TStringBuf JOKER_PROXY_FALLTHROUGH = "X-Yandex-Proxy-Header";

const TVector<TStringBuf> SERVICE_HEADERS = {
    JOKER_REQUEST_HEADER,
    JOKER_HOST_HEADER,
    JOKER_PROXY_HEADER,
    JOKER_PROXY_FAKE_TIME,
    JOKER_PROXY_FALLTHROUGH
};

NUri::TUri ConstructUri(const TParsedHttpFull& http) {
    NUri::TUri uri;
    if (uri.ParseUri(http.Request, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible) != NUri::TUri::EParsed::ParsedOK) {
        ythrow yexception() << "invalid or unsupported uri: " << http.Request;
    }
    return uri;
}

NUri::TUri ConstructForwardUri(const TMaybe<TString>& hostHeader) {
    NUri::TUri uri;
    if (hostHeader.Defined()) {
        if (uri.ParseUri(*hostHeader, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible) != NUri::TUri::EParsed::ParsedOK) {
            ythrow yexception() << "invalid or unsupported uri: " << *hostHeader;
        }
    }
    return uri;
}

THttpContext::THeaders ConstructHeaders(const THttpHeaders& headers, TMaybe<TString>* jokerHeader, TMaybe<TString>* hostHeader, TMaybe<TString>* proxyHeader) {
    Y_ASSERT(jokerHeader);
    Y_ASSERT(hostHeader);
    Y_ASSERT(proxyHeader);

    THttpContext::THeaders parsedHeaders;
    parsedHeaders.reserve(headers.Count());

    for (const auto& h : headers) {
        if (AsciiEqualsIgnoreCase(h.Name(), JOKER_REQUEST_HEADER) && !jokerHeader->Defined()) {
            jokerHeader->ConstructInPlace(h.Value());
        } else if (AsciiEqualsIgnoreCase(h.Name(), JOKER_HOST_HEADER) && !hostHeader->Defined()) {
            hostHeader->ConstructInPlace(h.Value());
        } else if (AsciiEqualsIgnoreCase(h.Name(), JOKER_PROXY_HEADER) && !proxyHeader->Defined()) {
            proxyHeader->ConstructInPlace(h.Value());
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
    , Headers_{ConstructHeaders(httpInput.Headers(), &JokerHeader_, &HostHeader_, &ProxyHeader_)}
    , Body_{httpInput.ReadAll()}
    , Uri_{ConstructUri(Http_)}
    , ForwardUri_{ConstructForwardUri(HostHeader_)}
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

TString THttpContext::CreateRequestHash() const {
    NOpenSsl::NSha256::TCalcer keyDigestCalcer;

    // Add Method and Path
    keyDigestCalcer.Update(Http_.Method);
    keyDigestCalcer.Update(ForwardUri_.PrintS(NUri::TField::EFlags::FlagAction));

    // Add CGI keys
    TVector<TString> cgiKeys;
    for (const auto& cgi : Cgi) {
        cgiKeys.emplace_back(cgi.first);
    }
    Sort(cgiKeys.begin(), cgiKeys.end());
    keyDigestCalcer.Update(JoinSeq("", cgiKeys));

    // Final calculation
    NOpenSsl::NSha256::TDigest digest = keyDigestCalcer.Final();
    return HexEncode(digest.data(), digest.size());
}

TString THttpContext::CreateHeadersDescription() const {
    TStringBuilder builder;
    builder << "Headers count: " << Headers_.size() << Endl;
    for (const auto& header : Headers_) {
        builder << header.Name() << ": " << header.Value() << Endl;
    }
    return builder;
}

bool IsServiceHeader(const TString& headerName) {
    for (const auto& serviceHeader : SERVICE_HEADERS) {
        if (AsciiHasPrefixIgnoreCase(headerName, serviceHeader)) {
            return true;
        }
    }
    return false;
}

} // namespace NAlice::NJokerLight
