#include "response.h"

#include "item_names.h"

#include <alice/library/network/headers.h>

#include <util/string/ascii.h>

namespace NAlice::NMegamind {

IHttpResponse& TAppHostHttpResponse::AddHeader(const THttpInputHeader& header) {
    Headers_.AddHeader(header);
    return *this;
}

IHttpResponse& TAppHostHttpResponse::SetContent(const TString& content) {
    Content_ = content;
    return *this;
}

IHttpResponse& TAppHostHttpResponse::SetContentType(TStringBuf contentType) {
    ContentType_ = contentType;
    return *this;
}

IHttpResponse& TAppHostHttpResponse::SetHttpCode(HttpCodes httpCode) {
    HttpCode_ = httpCode;
    return *this;
}

TString TAppHostHttpResponse::Content() const {
    return Content_;
}

HttpCodes TAppHostHttpResponse::HttpCode() const {
    return HttpCode_;
}

const THttpHeaders& TAppHostHttpResponse::Headers() const {
    return Headers_;
}

void TAppHostHttpResponse::DoOut() const {
    NJson::TJsonValue item;

    item[AH_ITEM_HTTP_CONTENT_JSFIELD] = Content_;
    item[AH_ITEM_HTTP_STATUS_CODE_JSFIELD] = HttpCode_;

    auto insertHeader = [&item](const THttpInputHeader& header) {
        NJson::TJsonValue jsonHeader;
        jsonHeader.AppendValue(header.Name());
        jsonHeader.AppendValue(header.Value());
        item[AH_ITEM_HTTP_HEADERS_CODE_JSFIELD].AppendValue(jsonHeader);
    };

    const bool needInsertContentType = !ContentType_.empty();
    for (const auto& header : Headers_) {
        if (AsciiEqualsIgnoreCase(header.Name(), NNetwork::HEADER_CONTENT_TYPE)) {
            if (needInsertContentType) {
                continue;
            }
        }

        insertHeader(header);
    }

    if (needInsertContentType) {
        insertHeader(THttpInputHeader{NNetwork::HEADER_CONTENT_TYPE, ContentType_});
    }

    ItemProxyAdapter_.PutJsonIntoContext(std::move(item), AH_ITEM_HTTP_RESPONSE);
}

} // namespace NAlice::NMegamind
