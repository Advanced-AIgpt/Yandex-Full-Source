#pragma once

#include <alice/megamind/library/util/http_response.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TErrorMetaBuilder {
public:
    using TMetaProto = TSpeechKitResponseProto::TMeta;

public:
    explicit TErrorMetaBuilder(const TError& error);
    explicit TErrorMetaBuilder(const TMetaProto& metaProto);
    explicit TErrorMetaBuilder(TMetaProto&& metaProto);

    TErrorMetaBuilder& SetOrigin(TMetaProto::EOrigin origin);

    TErrorMetaBuilder& SetNetLocation(TStringBuf netLocation);

    TErrorMetaBuilder& AppendNested(TMetaProto&& metaProto);
    TErrorMetaBuilder& AppendNested(TError&& error, TStringBuf netLocation);

    void ToHttpResponse(IHttpResponse& response, bool flush = true) const;
    NJson::TJsonValue AsJson() const;
    const TMetaProto& AsProtoItem() const;

    HttpCodes HttpCode() const;

private:
    TMetaProto MetaProto_;
};

} // namespace NAlice::NMegamind
