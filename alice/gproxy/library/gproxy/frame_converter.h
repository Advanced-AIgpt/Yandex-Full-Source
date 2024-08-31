#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/tv_feature_boarding/template.pb.h>
#include <alice/protos/data/video/card_detail.pb.h>
#include <alice/protos/data/tv/home/request.pb.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>

#include <alice/library/json/json.h>


namespace NGProxy {

using NGProxyService = NGProxyTraits::TGProxyService<NGProxy::GProxy>;

template<class TMethod, class TRequest>
TString ConvertToFrame(const TRequest& data) {
    return NAlice::JsonStringFromProto(data);
}

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvCardDetail, NAlice::TTvCardDetailsRequest>(const NAlice::TTvCardDetailsRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvFeatureBoarding, NAlice::NSmartTv::TTemplateRequest>(const NAlice::NSmartTv::TTemplateRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodReportTvFeatureBoardingTemplateShown, NAlice::NSmartTv::TTemplateRequest>(const NAlice::NSmartTv::TTemplateRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetTvSearchResult, NAlice::TTvSearchRequest>(const NAlice::TTvSearchRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetFakeGalleries, NAlice::TGetVideoGalleriesRequest>(const NAlice::TGetVideoGalleriesRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodGetFake2Galleries, NAlice::TGetVideoGalleriesRequest>(const NAlice::TGetVideoGalleriesRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodTvGetGalleries, NAlice::NTv::TGetGalleriesRequest>(const NAlice::NTv::TGetGalleriesRequest& data);

template<>
TString ConvertToFrame<NGProxyService::TServiceMethodTvGetGallery, NAlice::NTv::TGetGalleryRequest>(const NAlice::NTv::TGetGalleryRequest& data);

} // NGproxy