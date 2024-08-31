#include "search.h"
#include "helper.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/scenarios/video/scene/feature_calculator.h>
#include <alice/hollywood/library/scenarios/video/video_utils.h>
#include <alice/library/response_similarity/response_similarity.h>
#include <alice/hollywood/library/scenarios/video/scene/voice_buttons.h>
#include <alice/hollywood/library/services/response_merger/utils.h>

#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/div/div2card.pb.h>

namespace NAlice::NHollywoodFw::NVideo {

NScenarios::TTvOpenSearchScreenDirective MakeTvOpenSearchScreenDirective(const TVideoSearchResultArgs& args) {
    NScenarios::TTvOpenSearchScreenDirective directive;
    directive.SetSearchQuery(args.GetSearchText());
    return directive;
}

TRetMain TVideoOpenSearchScene::Main(const TVideoSearchResultArgs& _args,
                                     const TRunRequest& request,
                                     TStorage&,
                                     const TSource&) const {
    if (request.Client().GetClientInfo().IsCentaur()) {
        NRenderer::TDivRenderData divRenderData;
        divRenderData.SetCardId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
        divRenderData.MutableScenarioData()->MutableVideoSearchResultData()->CopyFrom(_args.GetSearchResultData());
        return TReturnValueRender(&TVideoOpenSearchScene::RenderSearchShowView, divRenderData).AddDivRender(std::move(divRenderData));
    }

    TVideoSearchResultArgs args = _args;
    auto& data = *args.MutableSearchResultData();

    if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_SETUP_TV_ACTIONS)) {
        TVideoVoiceButtons::SetupTvActions(data);
    }

    TMaybe<NVideoCommon::TVideoFeatures> videoFeatures;
    if (data.GetCarousels().size() > 0 && data.GetCarousels(0).HasBasicCarousel()) {
        videoFeatures = FillCarouselSimilarity(
            data.GetCarousels(0).GetBasicCarousel(),
            args.GetSearchText(),
            request.Input().GetUserLanguage());
    }

    TRunFeatures features = FillIntentData(request, false, videoFeatures.GetOrElse({}));

    if (const auto& slot = NHollywood::NVideo::TFrameSearchVideo(request.Input()).FindSlot("film_genre"); slot && slot->GetValue() == "porno" && request.User().IsSafeMode()) {
        LOG_INFO(request.Debug().Logger()) << "Searching for adult request in safe mode. Will warn about empty search: ";
        auto newArgs = TVideoSearchResultArgs();
        newArgs.SetSearchText(args.GetSearchText());
        newArgs.SetIsPorn(true);
        return TReturnValueRender(&TVideoOpenSearchScene::RenderSearch, newArgs, std::move(features));
    }
    return TReturnValueRender(&TVideoOpenSearchScene::RenderSearch, args, std::move(features));
}

TRetResponse TVideoOpenSearchScene::RenderSearchShowView(const NRenderer::TDivRenderData&, TRender& render) {
    auto& logger = render.GetRequest().Debug().Logger();

    NScenarios::TShowViewDirective showViewDirective;
    showViewDirective.SetName("show_view");
    showViewDirective.SetCardId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
    showViewDirective.MutableLayer()->MutableContent();
    showViewDirective.SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Long);

    if (const auto& divRenderResponses = render.GetDivRenderResponse(); !divRenderResponses.empty()) {
        LOG_DEBUG(logger) << "Got response from new render!";
        const auto divRenderResponse = std::find_if(divRenderResponses.begin(), divRenderResponses.end(),
            [](std::shared_ptr<NRenderer::TRenderResponse> resp) { return resp->GetCardId() == NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID;});
        if (divRenderResponse != divRenderResponses.end()) {
            *showViewDirective.MutableDiv2Card() = NHollywood::NResponseMerger::Div2CardFromRenderResponse(divRenderResponse->get(), logger);
        } else {
            LOG_ERR(logger) << "Div render response for " << NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID << " not found.";
        }
    } else {
        LOG_ERR(logger) << "Div render response is empty!";
    }

    render.Directives().AddShowViewDirective(std::move(showViewDirective));

    return TReturnValueSuccess();
}

TRetResponse TVideoOpenSearchScene::RenderSearch(const TVideoSearchResultArgs& args, TRender& render) const {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering search: " << args.GetSearchText();

    render.Directives().AddTvOpenSearchScreenDirective(MakeTvOpenSearchScreenDirective(args));
    if (!args.GetIsPorn()) {
        *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = NHollywood::NVideo::GetAnalyticsObjectForTvSearch(args.GetSearchResultData());
        render.CreateFromNlg("video", "gallery_opening", NJson::TJsonValue{});
    } else {
        render.CreateFromNlg("video", "all_results_filtered", NJson::TJsonValue{});
    }
    return TReturnValueSuccess();
}

TRetMain TVideoGRPCOpenSearchScene::Main(const TGetTvSearchResultSemanticFrame& frame, const TRunRequest& request, TStorage&, const TSource& source) const {
    auto args = TVideoSearchResultArgs();
    args.SetSearchText(frame.GetSearchText().GetStringValue());

    TTvSearchResultData searchData;
    if (!source.GetSource(NHollywood::NVideo::VIDEO__SEARCH_RESULT, searchData)) {
        TError err{TError::EErrorDefinition::SubsystemError};
        err.Details() << "No search_result data in scene";
        return err;
    }
    *args.MutableSearchResultData() = std::move(searchData);

    if (searchData.GetCarousels().size() > 0 && searchData.GetCarousels(0).HasBasicCarousel()) {
        if (const auto similarityFeatures = FillCarouselSimilarity(
            searchData.GetCarousels(0).GetBasicCarousel(),
            args.GetSearchText(),
            NHollywood::NVideo::GetLanguage(request)
            ); similarityFeatures.Defined())
        {
            LOG_INFO(request.Debug().Logger()) << "Filled features";
            TRunFeatures features;
            features.Set(similarityFeatures.GetRef());
            return TReturnValueRender(&TVideoGRPCOpenSearchScene::RenderSearch, args, std::move(features));
        }
    }
    return TReturnValueRender(&TVideoGRPCOpenSearchScene::RenderSearch, args);
}

TRetResponse TVideoGRPCOpenSearchScene::RenderSearch(const TVideoSearchResultArgs& args, TRender& render) const {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering search: " << args.GetSearchText();

    NScenarios::TCallbackDirective callbackDirective;
    callbackDirective.SetName("grpc_response");
    (*callbackDirective.MutablePayload()->mutable_fields())["grpc_response"].set_string_value(Base64Encode(args.GetSearchResultData().SerializeAsString()));

    *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = NHollywood::NVideo::GetAnalyticsObjectForTvSearch(args.GetSearchResultData());
    render.Directives().AddCallbackDirective(std::move(callbackDirective));
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NVideo
