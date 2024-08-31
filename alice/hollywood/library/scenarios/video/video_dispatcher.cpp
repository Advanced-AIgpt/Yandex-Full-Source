#include "video_dispatcher.h"

#include "item_selector.h"
#include "video_search.h"
#include "video_vh.h"
#include "web_search_helpers.h"
#include "search_metrics.h"

#include <alice/hollywood/library/scenarios/video/nlg/register.h>
#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>
#include <alice/hollywood/library/scenarios/video/scene/bass_proxy.h>
#include <alice/hollywood/library/scenarios/video/scene/card_details.h>
#include <alice/hollywood/library/scenarios/video/scene/general.h>
#include <alice/hollywood/library/scenarios/video/scene/open_item.h>
#include <alice/hollywood/library/scenarios/video/scene/play_video.h>
#include <alice/hollywood/library/scenarios/video/scene/search.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywoodFw::NVideo {

TVideoScenarioDispatch::TVideoScenarioDispatch()
    : TScenario("video")
{
    Register(&TVideoScenarioDispatch::DispatchSetup);
    Register(&TVideoScenarioDispatch::Dispatch);
    RegisterRenderer(&TVideoScenarioDispatch::RenderIrrelevant);
    RegisterScene<TVideoScenarioScene>([this]() {
        RegisterSceneFn(&TVideoScenarioScene::Main);
    });
    RegisterScene<TVideoCardDetailScene>([this]() {
        RegisterSceneFn(&TVideoCardDetailScene::Main);
        RegisterSceneFn(&TVideoCardDetailScene::ContinueSetup);
        RegisterSceneFn(&TVideoCardDetailScene::Continue);
    });
    RegisterScene<TVideoCardDetailThinScene>([this]() {
        RegisterSceneFn(&TVideoCardDetailThinScene::Main);
        RegisterSceneFn(&TVideoCardDetailThinScene::ContinueSetup);
        RegisterSceneFn(&TVideoCardDetailThinScene::Continue);
    });
    RegisterScene<TVideoPlayScene>([this]() {
        RegisterSceneFn(&TVideoPlayScene::MainSetup);
        RegisterSceneFn(&TVideoPlayScene::Main);
        RegisterRenderer(&TVideoPlayScene::PlayRender);
        RegisterRenderer(&TVideoPlayScene::DetailsScreenOpen);
        RegisterRenderer(&TVideoPlayScene::PirateVideoOpen);
    });
    RegisterScene<TVideoOpenSearchScene>([this]() {
        RegisterSceneFn(&TVideoOpenSearchScene::Main);
    });
    RegisterScene<TVideoGRPCOpenSearchScene>([this]() {
        RegisterSceneFn(&TVideoGRPCOpenSearchScene::Main);
    });
    RegisterScene<TVideoOpenItemScene>([this]() {
        RegisterSceneFn(&TVideoOpenItemScene::Main);
    });
    RegisterScene<TNoTvPluggedScene>([this]() {
        RegisterSceneFn(&TNoTvPluggedScene::Main);
    });
    RegisterScene<TVideoNotSupportedScene>([this]() {
        RegisterSceneFn(&TVideoNotSupportedScene::Main);
    });
    RegisterScene<TVideoBassProxy>([this]() {
        RegisterSceneFn(&TVideoBassProxy::RunSetup);
        RegisterSceneFn(&TVideoBassProxy::Main);
    });

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVideo::NNlg::RegisterAll);
    SetApphostGraph(ScenarioRequest() >>
                    NodePreselect("run/pre_select") >>
                    NodeRun("run/select") >>
                    NodeMain("run/main") >>
                    NodeRender("run/render") >>
                    ScenarioResponse());
    SetApphostGraph(ScenarioContinue() >>
                    NodeContinueSetup("continue_prepare") >>
                    NodeContinue("continue_render") >>
                    ScenarioResponse());
}

HW_REGISTER(TVideoScenarioDispatch);


static bool CanUseWebDS(const TRunRequest& runRequest, TRTLogger& logger) {
    if (runRequest.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ES)) {
        const auto baseInfo = WebSearchHelpers::ParseBaseInfoFromWeb(runRequest, logger);
        if (baseInfo.Defined()) {
            return true;
        }
    }
    if (runRequest.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ALL)) {
        if (WebSearchHelpers::HasUsefulSnippet(runRequest, logger)) {
            return true;
        }
    }
    return false;
}

TRetSetup TVideoScenarioDispatch::DispatchSetup(const TRunRequest& runRequest, const TStorage&) const {
    auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
    auto& logger = runRequest.Debug().Logger();
    Y_ENSURE(deviceState, "Unable to get TDeviceState");

    TSetup setup(runRequest);

    // flag video_use_pure_hw or video_pure_hw_gallery_selection needed here
    if ((runRequest.Input().HasSemanticFrame(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER) ||
            runRequest.Input().HasSemanticFrame(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT)) &&
        NVideoCommon::IsTvItemSelectionAvailable(*deviceState) && NHollywood::NVideo::IsTvOrModuleOrTandemRequest(runRequest, *deviceState)) {
        if (TMaybe<NHollywood::NVideo::ItemSelector> selection = NHollywood::NVideo::ItemSelector::TrySelectFromGallery(runRequest, *deviceState)) {
            setup.AttachRequest(NHollywood::NVideo::VIDEO__PRESELECT_DECISION, selection->MakePreselectArgs());
            return setup;
        } else {
            LOG_INFO(logger) << "Could not preselect for gallery.";
        }
    }

    if (runRequest.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ES)
        || runRequest.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ALL)) {
        NSearchMetrics::TrackWebSearchEntitySnippet(runRequest);
    }

    // flag video_pure_hw_search or video_pure_hw_base_info or video_use_pure_hw needed here
    NHollywood::NVideo::TFrameSearchVideo frameSearch(runRequest.Input(), false);
    if (frameSearch.Defined() && frameSearch.HasSearchText() &&
        (NHollywood::NVideo::IsTvOrModuleOrTandemRequest(runRequest, *deviceState)
        || runRequest.Client().GetClientInfo().IsCentaur() && !GetShowViewSceneData(frameSearch)))
    {

        if (CanUseWebDS(runRequest, logger)) {
            return TSetup(runRequest);
        }

        const auto searchArgs = MakeSearchCallArgs(frameSearch.SearchText, logger);
        setup.AttachRequest(NHollywood::NVideo::VIDEO_SEARCH_CALL, searchArgs); // TODO search restrictions
        return setup;
    }
    TGetTvSearchResultSemanticFrame getTvSearchResultTsf;
    if (runRequest.Input().FindTSF(NVideoCommon::TV_GET_SEARCH_RESULT, getTvSearchResultTsf)) {

        if (CanUseWebDS(runRequest, logger)) {
            return TSetup(runRequest);
        }

        const auto searchArgs = MakeSearchCallArgs(getTvSearchResultTsf.GetSearchText().GetStringValue(), logger, getTvSearchResultTsf.GetRestrictionMode().GetStringValue(), getTvSearchResultTsf.GetRestrictionAge().GetStringValue(), getTvSearchResultTsf.GetSearchEntref().GetStringValue());
        setup.AttachRequest(NHollywood::NVideo::VIDEO_SEARCH_CALL, searchArgs);
        return setup;
    }
    // No network request need, go to dispatcher immediate
    return setup;
}

TRetScene TVideoScenarioDispatch::DispatchSceneForBaseInfo(const NHollywood::NVideo::TFrameSearchVideo& frameSearch, const NTv::TCarouselItemWrapper& baseInfo, TRTLogger& logger) const {
    if (IsVideoPlayNeeded(frameSearch) || frameSearch.GetEpisode() || frameSearch.GetSeason()) {
        LOG_INFO(logger) << "Dispatched video play scene!";
        return TReturnValueScene<TVideoPlayScene>(NHollywood::NVideo::MakeVhArgs(frameSearch, baseInfo, logger), frameSearch.GetName());
    } else {
        LOG_INFO(logger) << "Dispatched open details scene!";
        TOpenItemSceneArgs args;
        *args.MutableCarouselItem() = std::move(baseInfo);
        return TReturnValueScene<TVideoOpenItemScene>(args, frameSearch.GetName());
    }
}

TRetScene TVideoScenarioDispatch::Dispatch(const TRunRequest& request, const TStorage&, const TSource& source) const {
    if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_BASS_ONLY)) return TReturnValueScene<TVideoBassProxy>(TBassProxySceneArgs{});

    TRTLogger& logger = request.Debug().Logger();
    auto deviceState = request.Client().TryGetMessage<TDeviceState>();
    Y_ENSURE(deviceState, "Unable to get TDeviceState");
    NHollywood::NVideo::TFrameSearchVideo frameSearch(request.Input());
    auto& sensors = request.System().GetSensors();

    // out-of rule dispatches
    if (request.Client().GetClientInfo().IsCentaur()) {
        if (auto showViewSceneData = GetShowViewSceneData(frameSearch)) {
            LOG_INFO(logger) << "Dispatched GetShowViewSceneData";
            TOpenItemSceneArgs args;
            *args.MutableShowViewSceneData() = std::move(*showViewSceneData);
            return TReturnValueScene<TVideoOpenItemScene>(args);
        } else if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_CENTAUR_VIDEOSEARCH_RESULT) && frameSearch.HasSearchText()) {
            LOG_INFO(logger) << "Dispatched Centaur search scene!";
            TVideoSearchResultArgs args;
            args.SetSearchText(frameSearch.SearchText);

            if (TTvSearchResultData data; source.GetSource(NHollywood::NVideo::VIDEO__SEARCH_RESULT, data)) {
                LOG_INFO(logger) << "Got Centaur video search data";
                *args.MutableSearchResultData() = std::move(data);
                return TReturnValueScene<TVideoOpenSearchScene>(args, frameSearch.GetName());
            } else {
                TError err{TError::EErrorDefinition::SubsystemError};
                err.Details() << "Empty Centaur video search data";
                return err;
            }
        }
    }
    {
        TOpenItemSceneArgs preselect;
        if (source.GetSource(NHollywood::NVideo::VIDEO__PRESELECT_DECISION, preselect)) {
            if (preselect.HasVideoItem() && NHollywood::NVideo::IsOttVideoItem(preselect.GetVideoItem()) && IsVideoPlayNeeded(frameSearch)) {
                LOG_INFO(logger) << "Dispatched play item from gallery";
                return TReturnValueScene<TVideoPlayScene>(NHollywood::NVideo::MakeVhArgs(frameSearch, preselect.GetVideoItem(), logger), preselect.GetPreselectedInfo().GetIntent());
            } else {
                LOG_INFO(logger) << "Dispatched opening item from gallery";
                return TReturnValueScene<TVideoOpenItemScene>(preselect, preselect.GetPreselectedInfo().GetIntent());
            }
        }
    }

    // start of Droideka proxies
    if (TFrameGetCardDetails frameGetCardDetails(request.Input()); frameGetCardDetails.Defined()) {
        return TReturnValueScene<TVideoCardDetailScene>(TVideoCardDetailScene::MakeVideoCardDetailSceneArgs(frameGetCardDetails));
    }
    if (TFrameGetCardDetailsThin frameGetCardDetailsThin(request.Input()); frameGetCardDetailsThin.Defined()) {
        return TReturnValueScene<TVideoCardDetailThinScene>(TVideoCardDetailThinScene::MakeVideoCardDetailThinSceneArgs(frameGetCardDetailsThin));
    }

    // start of TV, Module, Tandem and Centaur block
    if (NHollywood::NVideo::IsTvOrModuleOrTandemRequest(request, *deviceState) ||
        NHollywood::NVideo::IsCentaurRequest(request) && request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_CENTAUR_VIDEOSEARCH_RESULT)) {
        if (TGetTvSearchResultSemanticFrame getTvSearchResultTsf; request.Input().FindTSF(NVideoCommon::TV_GET_SEARCH_RESULT, getTvSearchResultTsf)) {
            return TReturnValueScene<TVideoGRPCOpenSearchScene>(getTvSearchResultTsf, ToString(NVideoCommon::TV_GET_SEARCH_RESULT));
        }
        if (NVideoCommon::IsContentDetailsScreen(*deviceState) && request.Input().FindSemanticFrame(NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO)) {
            TVideoItem item = NHollywood::NVideo::GetVideoItemFromContentDetailsScreen(*deviceState);
            LOG_INFO(logger) << "Dispatched play from content details scene!";
            return TReturnValueScene<TVideoPlayScene>(NHollywood::NVideo::MakeVhArgs(frameSearch, item, logger), ToString(NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO));
        }

        if (TGalleryVideoSelectSemanticFrame galleryVideoSelectTsf; request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_GALLERY_VIDEO_SELECT) && request.Input().FindTSF(NVideoCommon::VIDEO_GALLERY_SELECT, galleryVideoSelectTsf)) {
            if (galleryVideoSelectTsf.HasProviderItemId()) {
                if (ToString(galleryVideoSelectTsf.action().GetStringValue()) == "play") {
                    return TReturnValueScene<TVideoPlayScene>(NHollywood::NVideo::MakeSelectionVhArgs(galleryVideoSelectTsf), ToString(NVideoCommon::VIDEO_GALLERY_SELECT));
                } else {
                    TOttVideoItem item;
                    item.SetProviderItemId(galleryVideoSelectTsf.GetProviderItemId().GetStringValue());
                    TOpenItemSceneArgs args;
                    *args.MutableCarouselItem()->MutableVideoItem() = std::move(item);
                    return TReturnValueScene<TVideoOpenItemScene>(args, ToString(NVideoCommon::VIDEO_GALLERY_SELECT));
                }
            } else {
                TVideoItem item;
                item.SetEmbedUri(galleryVideoSelectTsf.GetEmbedUri().GetStringValue());
                TOpenItemSceneArgs args;
                *args.MutableVideoItem() = std::move(item);
                return TReturnValueScene<TVideoOpenItemScene>(args, ToString(NVideoCommon::VIDEO_GALLERY_SELECT));
            }
        }

        if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ES)) {
            const auto maybeBaseInfo = WebSearchHelpers::ParseBaseInfoFromWeb(request, logger);
            if (maybeBaseInfo.Defined()) {
                LOG_INFO(logger) << "Use base_info from WEB";
                NSearchMetrics::TrackWhichSearchResultWasUsed(sensors, NSearchMetrics::ESearchResultSource::WebSearchBaseInfo);
                return DispatchSceneForBaseInfo(frameSearch, *maybeBaseInfo, logger);
            }
        }

        if (NTv::TCarouselItemWrapper baseInfo; source.GetSource(NHollywood::NVideo::VIDEO__SEARCH_BASE_INFO, baseInfo)) {
            LOG_INFO(logger) << "Got base_info from search";
            NSearchMetrics::TrackWhichSearchResultWasUsed(sensors, NSearchMetrics::ESearchResultSource::VideoSearchBaseInfo);
            return DispatchSceneForBaseInfo(frameSearch, baseInfo, logger);
        }

        if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_WEB_SEARCH_ALL) && frameSearch.HasSearchText()) {
            TTvSearchResultData webSearchData = WebSearchHelpers::ParseSearchSnippetFromWeb(request, logger);
            if (webSearchData.GalleriesSize()) {
                NSearchMetrics::TrackWhichSearchResultWasUsed(sensors, NSearchMetrics::ESearchResultSource::WebSearchAll);
                LOG_INFO(logger) << "Use search results from WEB";
                TVideoSearchResultArgs args;
                args.SetSearchText(frameSearch.SearchText);
                *args.MutableSearchResultData() = std::move(webSearchData);
                return TReturnValueScene<TVideoOpenSearchScene>(args, frameSearch.GetName());
            }
        }

        if (frameSearch.HasSearchText()) {
            LOG_INFO(logger) << "Dispatched search scene!";
            TVideoSearchResultArgs args;
            args.SetSearchText(frameSearch.SearchText);

            if (TTvSearchResultData data; source.GetSource(NHollywood::NVideo::VIDEO__SEARCH_RESULT, data)) {
                LOG_INFO(logger) << "Got search data";
                NSearchMetrics::TrackWhichSearchResultWasUsed(sensors, NSearchMetrics::ESearchResultSource::VideoSearchAll);
                *args.MutableSearchResultData() = std::move(data);
            } else {
                LOG_WARN(logger) << "Empty search data";
            }
            return TReturnValueScene<TVideoOpenSearchScene>(args, frameSearch.GetName());
        } else {
            LOG_ERROR(logger) << "Empty search text!";
        }

        return TReturnValueRenderIrrelevant(&TVideoScenarioDispatch::RenderIrrelevant, {});
    } else {
        LOG_INFO(logger) << "Main section of Alice VideoScenario skipped because of not tv/module/tandem device or no exp flags";
    }

    if (request.Flags().IsExperimentEnabled(NVideoCommon::FLAG_VIDEO_USE_PURE_PLUGS)) {
        if (!request.Client().GetClientInfo().IsSmartSpeaker() && !request.Client().GetClientInfo().IsTvDevice()) {
            LOG_INFO(logger) << "Device is not smart speaker or tv device.";
            return TReturnValueScene<TVideoNotSupportedScene>(TNotSupportedSceneArgs{});
        } else if (request.Client().GetClientInfo().IsSmartSpeaker() && !deviceState->GetIsTvPluggedIn()) {
            if (!request.Client().GetInterfaces().GetSupportsHDMIOutput()) {
                return TReturnValueScene<TVideoNotSupportedScene>(TNotSupportedSceneArgs{});
            }
            return TReturnValueScene<TNoTvPluggedScene>(TNoTvPluggedSceneArgs{});
        }
    }

    return TReturnValueScene<TVideoBassProxy>(TBassProxySceneArgs{});
}

TRetResponse TVideoScenarioDispatch::RenderIrrelevant(const TRenderIrrelevant&, TRender& render) {
        render.CreateFromNlg("video", "error", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }

} // namespace NAlice::NHollywoodFw::NVideo
