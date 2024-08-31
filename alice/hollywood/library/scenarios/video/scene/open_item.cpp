#include "open_item.h"

#include "helper.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/scenarios/video/video_utils.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/centaur/webview.pb.h>
#include <alice/protos/div/div2card.pb.h>

namespace NAlice::NHollywoodFw::NVideo {

    NRenderer::TDivRenderData MakeDivRendererData(const TShowViewSceneData& data) {
        NRenderer::TDivRenderData divRendererData;

        divRendererData.SetCardId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);

        auto& webviewData = *divRendererData.MutableScenarioData()->MutableCentaurWebviewData();
        webviewData.SetId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
        webviewData.SetWebviewUrl(data.GetWebviewUrl());
        webviewData.SetShowNavigationBar(true);
        webviewData.SetMediaSessionId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);

        return divRendererData;
    }

    NScenarios::TVideoPlayDirective MakeVideoPlayDirective(const TVideoItem& item) {
        // for web video items
        NScenarios::TVideoPlayDirective directive;
        directive.SetName(item.GetName());
        directive.SetUri(item.GetEmbedUri());
        *directive.MutableItem() = item;
        if (!item.GetTvShowItemId().Empty()) {
            directive.MutableTvShowItem()->SetTvShowItemId(item.GetTvShowItemId());
            directive.MutableTvShowItem()->SetTvShowSeasonId(item.GetTvShowSeasonId());
        }

        return directive;
    }

    TRetMain TVideoOpenItemScene::Main(const TOpenItemSceneArgs& args, const TRunRequest& request, TStorage&, const TSource&) const {
        if (args.HasShowViewSceneData() && request.Client().GetClientInfo().IsCentaur()) {
            auto ret = TReturnValueRender(&TVideoOpenItemScene::OpenCentaurVideo, MakeDivRendererData(args.GetShowViewSceneData()));

            NScenarios::TShowViewDirective showViewDirective;
            showViewDirective.SetName("show_view");
            showViewDirective.SetCardId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
            showViewDirective.MutableLayer()->MutableContent();
            showViewDirective.SetDoNotShowCloseButton(true);
            showViewDirective.SetActionSpaceId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
            showViewDirective.SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Infinity);
            ret.AddDivRender(MakeDivRendererData(args.GetShowViewSceneData()));
            return ret;
        }

        NVideoCommon::TVideoFeatures preFeatures;
        if (args.GetPreselectedInfo().GetConfidence()) {
            preFeatures.SetItemSelectorConfidence(args.GetPreselectedInfo().GetConfidence());
            if (request.AI().GetIntent() == NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER) {
                preFeatures.SetItemSelectorConfidenceByNumber(args.GetPreselectedInfo().GetConfidence());
            } else if (request.AI().GetIntent() == NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT) {
                preFeatures.SetItemSelectorConfidenceByName(args.GetPreselectedInfo().GetConfidence());
            }
            preFeatures.SetIsGoToVideoScreen(1);
        }

        TRunFeatures features = FillIntentData(request, false, preFeatures);
        if (args.GetCarouselItem().HasPersonItem()) {
            return TReturnValueRender(&TVideoOpenItemScene::OpenPerson, args.GetCarouselItem().GetPersonItem(), std::move(features));
        } else if (args.GetCarouselItem().HasCollectionItem()) {
            return TReturnValueRender(&TVideoOpenItemScene::OpenCollection, args.GetCarouselItem().GetCollectionItem(), std::move(features));
        } else if (args.GetCarouselItem().HasVideoItem()) { // only OTT-videos (from search)
            return TReturnValueRender(&TVideoOpenItemScene::OpenOttVideo, args.GetCarouselItem().GetVideoItem(), std::move(features));
        } else if (args.HasVideoItem()) { // OTT and Web (from preselection)
            return TReturnValueRender(&TVideoOpenItemScene::OpenVideo, args.GetVideoItem(), std::move(features));
        } else {
            TError err(TError::EErrorDefinition::Unknown);
            err.Details() << "Unknown item in TVideoOpenItemScene::Main : " << args.GetCarouselItem().ShortUtf8DebugString();
            return err;
        }
    }

    void AddMediaSessionAction(NScenarios::TActionSpace& actionSpace, NScenarios::TActionSpace_TAction& action,
                               const TString& actionId, const TString& semanticFrameName) {
        auto& analytics = *action.MutableSemanticFrame()->MutableAnalytics();
        analytics.SetProductScenario(NAlice::NProductScenarios::VIDEO);
        analytics.SetPurpose(actionId);
        analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

        auto& nluHint = *actionSpace.AddNluHints();
        nluHint.SetActionId(actionId);
        nluHint.SetSemanticFrameName(semanticFrameName);

        (*actionSpace.MutableActions())[actionId] = action;
    }

    TRetResponse TVideoOpenItemScene::OpenCentaurVideo(const NRenderer::TDivRenderData&, TRender& render) const {
        NScenarios::TShowViewDirective showViewDirective;
        showViewDirective.SetName("show_view");
        showViewDirective.SetCardId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
        showViewDirective.MutableLayer()->MutableContent();
        showViewDirective.SetDoNotShowCloseButton(true);
        showViewDirective.SetActionSpaceId(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
        showViewDirective.SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Infinity);

        NScenarios::TActionSpace actionSpace;
        {
            NScenarios::TActionSpace_TAction action;
            action.MutableSemanticFrame()->MutableTypedSemanticFrame()->MutableMediaSessionPlaySemanticFrame()->MutableMediaSessionId()->SetStringValue(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
            AddMediaSessionAction(actionSpace, action, "youtube_mediasession_play", "personal_assistant.scenarios.player.continue");
        }
        {
            NScenarios::TActionSpace_TAction action;
            action.MutableSemanticFrame()->MutableTypedSemanticFrame()->MutableMediaSessionPauseSemanticFrame()->MutableMediaSessionId()->SetStringValue(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID);
            AddMediaSessionAction(actionSpace, action, "youtube_mediasession_pause", "personal_assistant.scenarios.player.pause");
        }
        render.AddActionSpace(NHollywood::NVideo::SHOW_VIEW_DIV_CARD_ID, std::move(actionSpace));

        if (auto& divRenderResponse = render.GetDivRenderResponse(); !divRenderResponse.empty()) {
            LOG_DEBUG(render.GetRequest().Debug().Logger()) << "Got response from new render!";
            TDiv2Card div2Card;
            *div2Card.MutableBody() = *divRenderResponse.back()->MutableDiv2Body();
            *showViewDirective.MutableDiv2Card() = div2Card;
        } else {
            LOG_ERR(render.GetRequest().Debug().Logger()) << "Div render response is empty!";
        }

        render.Directives().AddShowViewDirective(std::move(showViewDirective));
        return TReturnValueSuccess();
    }

    TRetResponse TVideoOpenItemScene::OpenOttVideo(const TOttVideoItem& item, TRender& render) const {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Opening details screen " << item.GetTitle() << " id: " << item.GetProviderItemId();
        render.Directives().AddTvOpenDetailsScreenDirective(NHollywood::NVideo::MakeTvOpenDetailsScreenDirective(item));
        render.CreateFromNlg("video", "show_video_description", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }

    TRetResponse TVideoOpenItemScene::OpenVideo(const TVideoItem& item, TRender& render) const {
        if (NHollywood::NVideo::IsOttVideoItem(item)) {
            LOG_INFO(render.GetRequest().Debug().Logger()) << "Opening details screen " << item.GetName() << " id: " << item.GetProviderItemId();
            *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = NAlice::NHollywood::NVideo::GetAnalyticsObjectForDescription(item, false);
            render.Directives().AddTvOpenDetailsScreenDirective(NHollywood::NVideo::MakeTvOpenDetailsScreenDirective(item));
            render.CreateFromNlg("video", "show_video_description", NJson::TJsonValue{});
            return TReturnValueSuccess();
        } else {
            LOG_INFO(render.GetRequest().Debug().Logger()) << "Playing web item " << item.GetName();
            *render.GetResponseBody().MutableAnalyticsInfo()->AddObjects() = NAlice::NHollywood::NVideo::GetAnalyticsObjectCurrentlyPlayingVideo(item);
            render.Directives().AddVideoPlayDirective(MakeVideoPlayDirective(item));
            render.CreateFromNlg("video", "video_autoplay", NJson::TJsonValue{});
            return TReturnValueSuccess();
        }
    }

    TRetResponse TVideoOpenItemScene::OpenPerson(const TPersonItem& person, TRender& render) {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Opening person " << person.GetName();
        render.Directives().AddTvOpenPersonScreenDirective(NHollywood::NVideo::MakeTvOpenPersonScreenDirective(person));
        render.CreateFromNlg("video", "open_card", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }

    TRetResponse TVideoOpenItemScene::OpenCollection(const TCollectionItem& collection, TRender& render) {
        LOG_INFO(render.GetRequest().Debug().Logger()) << "Opening collection " << collection;
        render.Directives().AddTvOpenCollectionScreenDirective(NHollywood::NVideo::MakeTvOpenCollectionScreenDirective(collection));
        render.CreateFromNlg("video", "open_card", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }

} // namespace NAlice::NHollywoodFw::NVideo
