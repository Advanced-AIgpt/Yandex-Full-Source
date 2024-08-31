#include "error.h"

#include <alice/library/json/json.h>
#include <alice/library/network/common.h>

namespace NAlice::NMegamind {

TErrorMetaBuilder::TErrorMetaBuilder(const TError& error) {
    MetaProto_.SetType("error");
    MetaProto_.SetErrorType(ToString(error.Type));
    MetaProto_.SetMessage(error.ErrorMsg);
    MetaProto_.SetOrigin(TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Status);

    // TODO create a different implementation for uniproxy error type
    HttpCodes httpCode = HTTP_INTERNAL_SERVER_ERROR;
    switch (error.Type) {
        case TError::EType::Begemot:
            [[fallthrough]];
        case TError::EType::Biometry:
            [[fallthrough]];
        case TError::EType::DataError:
            [[fallthrough]];
        case TError::EType::Empty:
            [[fallthrough]];
        case TError::EType::Input:
            [[fallthrough]];
        case TError::EType::ModifierError:
            [[fallthrough]];
        case TError::EType::NetworkError:
            [[fallthrough]];
        case TError::EType::NLG:
            [[fallthrough]];
        case TError::EType::TimeOut:
            [[fallthrough]];
        case TError::EType::Unauthorized:
            [[fallthrough]];
        case TError::EType::UniProxy:
            [[fallthrough]];
        case TError::EType::NotFound:
            [[fallthrough]];
        case TError::EType::VersionMismatch:
            [[fallthrough]];
        case TError::EType::ScenarioError:
            httpCode = error.HttpCode.GetOrElse(HTTP_UNASSIGNED_512);
            break;

        case TError::EType::BadRequest:
            MetaProto_.SetOrigin(TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Exception);
            httpCode = error.HttpCode.GetOrElse(HTTP_BAD_REQUEST);
            break;

        case TError::EType::Exception:
            MetaProto_.SetOrigin(TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Exception);
            [[fallthrough]];
        case TError::EType::Critical:
            [[fallthrough]];
        case TError::EType::Http:
            [[fallthrough]];
        case TError::EType::Logic:
            [[fallthrough]];
        case TError::EType::Parse:
            [[fallthrough]];
        case TError::EType::System:
            httpCode = error.HttpCode.GetOrElse(HTTP_INTERNAL_SERVER_ERROR);
            break;
    }

    MetaProto_.SetHttpCode(httpCode);
}

TErrorMetaBuilder::TErrorMetaBuilder(const TMetaProto& metaProto)
    : MetaProto_{metaProto}
{
}

TErrorMetaBuilder::TErrorMetaBuilder(TMetaProto&& metaProto)
    : MetaProto_{std::move(metaProto)}
{
}

TErrorMetaBuilder& TErrorMetaBuilder::SetOrigin(TMetaProto::EOrigin origin) {
    MetaProto_.SetOrigin(origin);
    return *this;
}

TErrorMetaBuilder& TErrorMetaBuilder::SetNetLocation(TStringBuf netLocation) {
    MetaProto_.SetNetLocation(ToString(netLocation));
    return *this;
}

TErrorMetaBuilder& TErrorMetaBuilder::AppendNested(TMetaProto&& metaProto) {
    auto& nestedProto = *MetaProto_.AddNestedErrors();
    nestedProto.SetType(metaProto.GetErrorType());
    nestedProto.SetOrigin(metaProto.GetOrigin());
    nestedProto.SetMessage(metaProto.GetMessage());
    nestedProto.SetNetLocation(metaProto.GetNetLocation());
    return *this;
}

TErrorMetaBuilder& TErrorMetaBuilder::AppendNested(TError&& error, TStringBuf netLocation) {
    auto& nestedProto = *MetaProto_.AddNestedErrors();
    nestedProto.SetType(ToString(error.Type));
    nestedProto.SetMessage(error.ErrorMsg);
    nestedProto.SetNetLocation(ToString(netLocation));
    nestedProto.SetOrigin(TMetaProto::EOrigin::TSpeechKitResponseProto_TMeta_EOrigin_Status);
    return *this;
}

void TErrorMetaBuilder::ToHttpResponse(IHttpResponse& response, bool flush) const {
    TSpeechKitResponseErrorProto proto;
    *proto.MutableMeta()->Add() = MetaProto_;
    response.SetHttpCode(static_cast<HttpCodes>(MetaProto_.GetHttpCode()))
            .SetContentType(NContentTypes::APPLICATION_JSON)
            .SetContent(ToString(JsonFromProto(proto)));

    if (flush) {
        response.Flush();
    }
}

NJson::TJsonValue TErrorMetaBuilder::AsJson() const {
    return JsonFromProto(MetaProto_);
}

const TErrorMetaBuilder::TMetaProto& TErrorMetaBuilder::AsProtoItem() const {
    return MetaProto_;
}

HttpCodes TErrorMetaBuilder::HttpCode() const {
    return static_cast<HttpCodes>(MetaProto_.GetHttpCode());
}

} // namespace NAlice::NMegamind
