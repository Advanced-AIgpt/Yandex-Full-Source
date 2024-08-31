#include "frame_converter.h"

#include <alice/library/json/json.h>
#include <google/protobuf/message.h>

namespace NGProxy {

using NGProxyService = NGProxyTraits::TGProxyService<NGProxy::GProxy>;

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvCardDetail, NAlice::TTvCardDetailsRequest>(const NAlice::TTvCardDetailsRequest& data) {
    NAlice::TVideoCardDetailSemanticFrame req;

    req.MutableContentId()->SetStringValue(data.GetContentId());
    req.MutableContentOntoId()->SetStringValue(data.GetContentOntoId());
    req.MutableContentType()->SetStringValue(data.GetContentType());

    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvFeatureBoarding, NAlice::NSmartTv::TTemplateRequest>(const NAlice::NSmartTv::TTemplateRequest& data) {
    NAlice::TTvPromoTemplateRequestSemanticFrame req;
    req.MutableChosenTemplate()->MutableTandemTemplate()->CopyFrom(data.tandemtemplate());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodReportTvFeatureBoardingTemplateShown, NAlice::NSmartTv::TTemplateRequest>(const NAlice::NSmartTv::TTemplateRequest& data) {
    NAlice::TTvPromoTemplateShownReportSemanticFrame req;
    req.MutableChosenTemplate()->MutableTandemTemplate()->CopyFrom(data.tandemtemplate());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvSearchResult, NAlice::TTvSearchRequest>(const NAlice::TTvSearchRequest& data) {
    NAlice::TGetTvSearchResultSemanticFrame req;
    req.MutableSearchText()->SetStringValue(data.GetSearchText());
    req.MutableRestrictionMode()->SetStringValue(data.GetRestrictionMode());
    req.MutableRestrictionAge()->SetStringValue(data.GetRestrictionAge());
    req.MutableSearchEntref()->SetStringValue(data.GetEntref());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetFakeGalleries, NAlice::TGetVideoGalleriesRequest>(const NAlice::TGetVideoGalleriesRequest& data) {
    NAlice::TGetVideoGalleriesSemanticFrame req;
    req.MutableCategoryId()->SetStringValue(data.GetCategoryId());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetFake2Galleries, NAlice::TGetVideoGalleriesRequest>(const NAlice::TGetVideoGalleriesRequest& data) {
    NAlice::TGetVideoGalleriesSemanticFrame req;
    req.MutableCategoryId()->SetStringValue(data.GetCategoryId());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodTvGetGalleries, NAlice::NTv::TGetGalleriesRequest>(const NAlice::NTv::TGetGalleriesRequest& data) {
    NAlice::TGetVideoGalleriesSemanticFrame req;
    req.MutableCategoryId()->SetStringValue(data.GetCategoryId());
    req.MutableMaxItemsPerGallery()->SetNumValue(data.GetMaxItemsPerGallery());
    req.MutableOffset()->SetNumValue(data.GetOffset());
    req.MutableLimit()->SetNumValue(data.GetLimit());
    req.MutableCacheHash()->SetStringValue(data.GetCacheHash());
    req.MutableFromScreenId()->SetStringValue(data.GetFromScreenId());
    req.MutableParentFromScreenId()->SetStringValue(data.GetParentFromScreenId());
    req.MutableKidModeEnabled()->SetBoolValue(data.GetKidModeEnabled());
    req.MutableRestrictionAge()->SetStringValue(data.GetRestrictionAge());
    return NAlice::JsonStringFromProto(req);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodTvGetGallery, NAlice::NTv::TGetGalleryRequest>(const NAlice::NTv::TGetGalleryRequest& data) {
    NAlice::TGetVideoGallerySemanticFrame req;
    req.MutableId()->SetStringValue(data.GetId());
    req.MutableOffset()->SetNumValue(data.GetOffset());
    req.MutableLimit()->SetNumValue(data.GetLimit());
    req.MutableCacheHash()->SetStringValue(data.GetCacheHash());
    req.MutableFromScreenId()->SetStringValue(data.GetFromScreenId());
    req.MutableParentFromScreenId()->SetStringValue(data.GetParentFromScreenId());
    req.MutableCarouselPosition()->SetNumValue(data.GetCarouselPosition());
    req.MutableCarouselTitle()->SetStringValue(data.GetCarouselTitle());
    req.MutableKidModeEnabled()->SetBoolValue(data.GetKidModeEnabled());
    req.MutableRestrictionAge()->SetStringValue(data.GetRestrictionAge());
    req.MutableSelectedTags()->MutableCatalogTags()->MutableCatalogTagValue()->CopyFrom(data.GetSelectedTags());
    return NAlice::JsonStringFromProto(req);
}

} // NGProxy
