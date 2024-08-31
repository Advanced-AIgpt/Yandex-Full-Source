#include "http_request.h"

namespace NMatrix {

namespace {

TString GetCensoredString(const TString& str) {
    TString result = str;
    for (size_t i = result.size() / 2; i < result.size(); ++i) {
        result[i] = '*';
    }
    return result;
}

} // namespace

namespace NPrivate {

static constexpr TStringBuf APPHOST_REQUEST_ID_HEADER = "x-apphost-request-reqid";
static constexpr TStringBuf APPHOST_REQUEST_RUID_HEADER = "x-apphost-request-ruid";
static constexpr TStringBuf CONTENT_TYPE_HEADER = "content-type";
static constexpr TStringBuf REQUEST_ID_HEADER = "x-request-id";

static constexpr TStringBuf JSON_CONTENT_TYPE = "application/json";

static const TString EMPTY_REQUEST_ID = "empty-request-id";


THolder<NNeh::IHttpRequest> CastNehRequestPtrToNehHttpRequestPtr(const NNeh::IRequestRef& request) {
    auto* httpRequest = dynamic_cast<NNeh::IHttpRequest*>(request.Get());
    if (Y_LIKELY(httpRequest)) {
        Y_UNUSED(request.Release());
        return THolder<NNeh::IHttpRequest>(httpRequest);
    } else {
        throw yexception() << "Dynamic cast to NNeh::IHttpRequest* is failed";
    }
}

bool HttpRequestContentIsJson(const THttpHeaders& httpRequestHeaders) {
    const THttpInputHeader* contentType = httpRequestHeaders.FindHeader(CONTENT_TYPE_HEADER);
    return contentType && contentType->Value() == JSON_CONTENT_TYPE;
}

TString GetRequestIdFromHttpRequestHeaders(const THttpHeaders& httpRequestHeaders) {
    if (const THttpInputHeader* requestIdHeader = httpRequestHeaders.FindHeader(REQUEST_ID_HEADER)) {
        return requestIdHeader->Value();
    }

    if (const THttpInputHeader* apphostRequestIdHeader = httpRequestHeaders.FindHeader(APPHOST_REQUEST_ID_HEADER)) {
        if (const THttpInputHeader* apphostRuidHeader = httpRequestHeaders.FindHeader(APPHOST_REQUEST_RUID_HEADER)) {
            return TString::Join(apphostRequestIdHeader->Value(), '-', apphostRuidHeader->Value());
        } else {
            return apphostRequestIdHeader->Value();
        }
    }

    return EMPTY_REQUEST_ID;
}

TString GetCensoredHeaderValue(const TString& headerName, const TString& headerValue) {
    static const THashSet<TString> uncensorableHeaderList = {
        "accept",
        "accept-encoding",
        "connection",
        "content-length",
        "content-type",
        "host",
        "keep-alive",
        "user-agent",

        "x-apphost-callpath",
        "x-apphost-request-guid",
        "x-apphost-request-location",
        "x-apphost-request-reqid",
        "x-apphost-request-ruid",

        "x-solomon-clusterid",
        "x-solomon-fetcherid",
        "x-solomon-sequencenumber",
        "x-solomon-shard-limit",

        "x-forwarded-for",
        "x-real-ip",
        "x-request-id",
        "x-rtlog-token",
        "x-ya-rtlog-token",
        "x-ya-user-agent",
        "x-yandex-balancer-retry",
    };

    if (uncensorableHeaderList.contains(to_lower(headerName))) {
        return headerValue;
    } else {
        // Censore values of all unknown headers
        return GetCensoredString(headerValue);
    }
}

TString GetReplyHeadersString(const THttpHeaders& headers) {
    if (headers.Empty()) {
        return "";
    }

    TString headersString;
    TStringStream headersStream(headersString);

    headersStream << "\r\n";
    headers.OutTo(&headersStream);

    return headersStream.Str().substr(0, headersStream.Str().length() - 2);
}

} // namespace NPrivate

} // namespace NMatrix
