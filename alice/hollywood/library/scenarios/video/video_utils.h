#pragma once

#include "analytics.h"

#include <alice/hollywood/library/environment_state/environment_state.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>
#include <alice/library/network/headers.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>
#include <alice/library/video_common/hollywood_helpers/util.h>
#include <alice/library/video_common/protos/features.pb.h>
#include <alice/megamind/protos/common/tandem_state.pb.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/video/content_details.pb.h>
#include <alice/protos/data/video/tv_backend_request.pb.h>
#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NVideo {

inline constexpr TStringBuf VIDEO__PRESELECT_DECISION = "video__preselect_decision";
inline constexpr TStringBuf VIDEO__SEARCH_BASE_INFO = "video__search_base_info";
inline constexpr TStringBuf VIDEO__SEARCH_RESULT = "video__search_result";

inline constexpr TStringBuf VH_PROXY_REQUEST = "vh_proxy_request";
inline constexpr TStringBuf VH_PROXY_RESPONSE = "vh_proxy_response";

inline const TString VIDEO_SEARCH_CALL = "video_search_call";

inline static const TString YOUTUBE_WEBVIEW_URL = "https://www.youtube.com/";
inline static const TString SHOW_VIEW_DIV_CARD_ID = "div.show_view";


struct TFrameSearchVideo : public NHollywoodFw::TFrame {
    struct TFrameSearchVideoText : public NHollywoodFw::TFrame {
        TFrameSearchVideoText(const NHollywoodFw::TRequest::TInput& input, bool includeSeasonAndEpisode)
            : TFrame(input, NVideoCommon::SEARCH_VIDEO_TEXT)
            , Top(this, "top")
            , New(this, "new")
            , Free(this, "free")
            , ContentType(this, "content_type")
            , Genre(this, "film_genre")
            , Country(this, "country")
            , ReleaseDate(this, "release_date")
            , Season(this, "season")
            , Episode(this, "episode")
            , SearchText(this, "search_text")
            {
                for (const auto& slot : {Top, New, Free, ContentType, Genre, Country, ReleaseDate, SearchText}) {
                    if (slot.Defined()) {
                        if (Text) {
                            Text += ' ';
                        }
                        Text += *slot.Value;
                    }
                }

                // TODO improve localisation here
                if (includeSeasonAndEpisode) {
                    if (Season.Defined()) {
                        if (Text) {
                            Text += ' ';
                        }
                        Text += *Season.Value + " сезон";
                    }
                    if (Episode.Defined()) {
                        if (Text) {
                            Text += ' ';
                        }
                        Text += *Episode.Value + " серия";
                    }
                }
            }

    public:
        TString Text;

    private:
        NHollywoodFw::TOptionalSlot<TString> Top;
        NHollywoodFw::TOptionalSlot<TString> New;
        NHollywoodFw::TOptionalSlot<TString> Free;

        NHollywoodFw::TOptionalSlot<TString> ContentType;

        NHollywoodFw::TOptionalSlot<TString> Genre;
        NHollywoodFw::TOptionalSlot<TString> Country;
        NHollywoodFw::TOptionalSlot<TString> ReleaseDate;
        NHollywoodFw::TOptionalSlot<TString> Season;
        NHollywoodFw::TOptionalSlot<TString> Episode;

        NHollywoodFw::TOptionalSlot<TString> SearchText;
    };

    TFrameSearchVideo(const NHollywoodFw::TRequest::TInput& input, bool includeSeasonAndEpisode = true)
        : TFrame(input, NVideoCommon::SEARCH_VIDEO)
        , Action(this, "action")
        , ContentProvider(this, "content_provider")
        , FilmGenre(this, "film_genre")
        , Season(this, "season")
        , Episode(this, "episode")
        , SearchText(TFrameSearchVideoText(input, includeSeasonAndEpisode).Text)
    {
    }

    TMaybe<ui32> GetSeason() const {
        return Season ? Season.Value : Nothing();
    }

    TMaybe<ui32> GetEpisode() const {
        return Episode ? Episode.Value : Nothing();
    }

    bool HasSearchText() const {
        return !SearchText.Empty();
    }

    NHollywoodFw::TOptionalSlot<TString> Action;
    NHollywoodFw::TOptionalSlot<TString> ContentProvider;
    NHollywoodFw::TOptionalSlot<TString> FilmGenre;
    NHollywoodFw::TOptionalSlot<ui32> Season;
    NHollywoodFw::TOptionalSlot<ui32> Episode;

    TString SearchText;
};

inline TMaybe<NHollywoodFw::NVideo::TShowViewSceneData> GetShowViewSceneData(NHollywood::NVideo::TFrameSearchVideo& searchFrame) {
    if (searchFrame.Defined() && searchFrame.SearchText && searchFrame.ContentProvider.Value) {
        if (*searchFrame.ContentProvider.Value == "youtube") {
            NHollywoodFw::NVideo::TShowViewSceneData showViewSceneData;
            auto searchText = searchFrame.SearchText;
            TStringStream searchTextEncoded;
            NUri::TEncoder::EncodeNotAlnum(searchTextEncoded, searchText);
            showViewSceneData.SetWebviewUrl(YOUTUBE_WEBVIEW_URL + "results?search_query=" + searchTextEncoded.Str());
            return showViewSceneData;
        }
    }
    return Nothing();
}

inline bool IsVideoPlayNeeded(const TFrameSearchVideo& frameSearch) {
    if (frameSearch.Action.Defined()) {
        return *frameSearch.Action.Value == "play";
    }
    return false;
}

inline NHollywoodFw::NVideo::TVideoVhArgs MakeVhArgs(const TFrameSearchVideo& frameSearch, TMaybe<NTv::TCarouselItemWrapper> baseInfoMaybe, TRTLogger& logger) {
    NHollywoodFw::NVideo::TVideoVhArgs args;
    args.SetProviderItemId(baseInfoMaybe->GetVideoItem().GetProviderItemId());
    args.SetContentType(baseInfoMaybe->GetVideoItem().GetContentType());
    auto season = frameSearch.GetSeason();
    auto episode = frameSearch.GetEpisode();
    if (season || episode) {
        if (baseInfoMaybe->HasVideoItem() && baseInfoMaybe->GetVideoItem().GetContentType() == "tv_show") {
            if (auto season = frameSearch.GetSeason()) {
                args.SetHasSeason(1);
                args.SetSeason(*season);
            }
            if (auto episode = frameSearch.GetEpisode()) {
                args.SetHasEpisode(1);
                args.SetEpisode(*episode);
            }
        } else {
            LOG_WARN(logger) << "Base info has season/episode, but video item is not a tv_show: " << baseInfoMaybe->DebugString();
        }
    }
    return args;
}

inline NHollywoodFw::NVideo::TVideoVhArgs MakeVhArgs(const TFrameSearchVideo& frameSearch, TVideoItem videoItem, TRTLogger& logger) {
    NHollywoodFw::NVideo::TVideoVhArgs args;
    args.SetProviderItemId(videoItem.GetProviderItemId());
    args.SetContentType(videoItem.GetType());
    auto season = frameSearch.GetSeason();
    auto episode = frameSearch.GetEpisode();
    if (season || episode) {
        if (videoItem.GetType() == "tv_show") {
            if (auto season = frameSearch.GetSeason()) {
                args.SetHasSeason(1);
                args.SetSeason(*season);
            }
            if (auto episode = frameSearch.GetEpisode()) {
                args.SetHasEpisode(1);
                args.SetEpisode(*episode);
            }
        } else {
            LOG_WARN(logger) << "Base info has season/episode, but video item is not a tv_show: " << videoItem.ShortUtf8DebugString();
        }
    }
    return args;
}

inline NHollywoodFw::NVideo::TVideoVhArgs MakeSelectionVhArgs(const TGalleryVideoSelectSemanticFrame& galleryVideoSelectTsf) {
    NHollywoodFw::NVideo::TVideoVhArgs args;
    args.SetProviderItemId(galleryVideoSelectTsf.GetProviderItemId().GetStringValue());
    args.SetContentType("FILM");
    return args;
}

inline NScenarios::TTvOpenSearchScreenDirective MakeTvOpenSearchScreenDirective(const TVideoItem& videoItem) {
    NScenarios::TTvOpenSearchScreenDirective directive;
    directive.SetSearchQuery(videoItem.GetSearchQuery());
    return directive;
}

inline NScenarios::TTvOpenPersonScreenDirective MakeTvOpenPersonScreenDirective(const TPersonItem& personItem) {
    NScenarios::TTvOpenPersonScreenDirective directive;
    directive.SetKpId(personItem.GetKpId());
    directive.MutableData()->SetName(personItem.GetName());
    directive.MutableData()->SetSubtitle(personItem.GetSubtitle());
    if (personItem.HasImage()) {
        *directive.MutableData()->MutableImage() = personItem.GetImage();
    }
    return directive;
}

inline NScenarios::TTvOpenCollectionScreenDirective MakeTvOpenCollectionScreenDirective(const TCollectionItem& collectionItem) {
    NScenarios::TTvOpenCollectionScreenDirective directive;
    directive.SetSearchQuery(collectionItem.GetSearchQuery());
    directive.SetEntref(collectionItem.GetEntref());
    directive.MutableData()->SetTitle(collectionItem.GetTitle());
    return directive;
}

inline NScenarios::TTvOpenDetailsScreenDirective MakeTvOpenDetailsScreenDirective(const TVideoItem& videoItem) {
    NScenarios::TTvOpenDetailsScreenDirective directive;
    if (videoItem.GetType() == "movie") {
        directive.SetContentType("MOVIE");
        directive.SetVhUuid(videoItem.GetProviderItemId());
    } else if (videoItem.GetType() == "tv_show") {
        directive.SetContentType("TV_SERIES");
        directive.SetVhUuid(videoItem.GetProviderItemId());
    } else if (videoItem.GetType() == "tv_show_episode") {
        directive.SetContentType("TV_SERIES");
        directive.SetVhUuid(videoItem.GetTvShowItemId());
    }

    directive.SetSearchQuery(videoItem.GetSearchQuery());

    auto& data = *directive.MutableData();
    data.SetName(videoItem.GetName());
    data.SetDescription(videoItem.GetDescription());
    data.SetName(videoItem.GetName());
    data.SetMinAge(videoItem.GetMinAge());
    if (videoItem.HasThumbnail()) {
        *data.MutableThumbnail() = videoItem.GetThumbnail();
    }
    if (videoItem.HasPoster()) {
        *data.MutablePoster() = videoItem.GetPoster();
    }
    return directive;
}
inline NScenarios::TTvOpenDetailsScreenDirective MakeTvOpenDetailsScreenDirective(const TOttVideoItem& ottItem) {
    NScenarios::TTvOpenDetailsScreenDirective directive;
    if (ottItem.GetContentType() == "movie") {
        directive.SetContentType("MOVIE");
    } else if (ottItem.GetContentType() == "tv_show") {
        directive.SetContentType("TV_SERIES");
    }

    directive.SetVhUuid(ottItem.GetProviderItemId());
    directive.SetSearchQuery(ottItem.GetSearchQuery());

    auto& data = *directive.MutableData();
    data.SetName(ottItem.GetTitle());
    data.SetDescription(ottItem.GetDescription());
    data.SetName(ottItem.GetTitle());
    if (ottItem.HasThumbnail()) {
        *data.MutableThumbnail() = ottItem.GetThumbnail();
    }
    if (ottItem.HasPoster()) {
        *data.MutablePoster() = ottItem.GetPoster();
    }
    return directive;
}

inline bool IsOttVideoItem(const TVideoItem& item) {
    return item.GetProviderName() == NVideoCommon::PROVIDER_KINOPOISK;
}

inline bool IsTandemEnabled(const TScenarioBaseRequestWrapper& request) {
    return request.BaseRequestProto().GetDeviceState().HasTandemState() && request.BaseRequestProto().GetDeviceState().GetTandemState().GetConnected();
}
inline bool IsTandemEnabled(const TDeviceState& deviceState) {
    return deviceState.HasTandemState() && deviceState.GetTandemState().GetConnected();
}

inline bool IsTvOrModuleOrTandemRequest(const TScenarioBaseRequestWrapper& request) {
    return request.ClientInfo().IsTvDevice() || request.ClientInfo().IsYaModule() || IsTandemEnabled(request);
}
inline bool IsTvOrModuleOrTandemRequest(const NHollywoodFw::TRunRequest& request, const TDeviceState& deviceState) {
    return request.Client().GetClientInfo().IsTvDevice() || request.Client().GetClientInfo().IsYaModule() || IsTandemEnabled(deviceState);
}
inline bool IsCentaurRequest(const NHollywoodFw::TRunRequest& request) {
    return request.Client().GetClientInfo().IsCentaur();
}

inline ELanguage GetLanguage(const NHollywoodFw::TRunRequest& runRequest) {
    const ELanguage lang = LanguageByName(runRequest.Client().GetClientInfo().Lang);
    if (lang == ELanguage::LANG_UNK) {
        return ELanguage::LANG_RUS;
    }
    return lang;
}

inline TVideoItem GetVideoItemFromContentDetailsScreen(const TDeviceState& deviceState) {
    const auto& contentDetailsitem = deviceState.GetVideo().GetTvInterfaceState().GetContentDetailsScreen().GetCurrentItem();
    TVideoItem videoItem;
    videoItem.SetProviderItemId(contentDetailsitem.GetProviderItemId());
    videoItem.SetProviderName(contentDetailsitem.GetProviderName());
    videoItem.SetType(contentDetailsitem.GetItemType());
    videoItem.SetAvailable(true);
    videoItem.SetSearchQuery(contentDetailsitem.GetSearchQuery());
    if (contentDetailsitem.HasAgeLimit()) {
        videoItem.SetMinAge(contentDetailsitem.GetAgeLimit());
        videoItem.SetAgeLimit(ToString(contentDetailsitem.GetAgeLimit()));
    }
    videoItem.AddProviderInfo()->SetType(videoItem.GetType());
    videoItem.AddProviderInfo()->SetProviderItemId(contentDetailsitem.GetProviderItemId());
    videoItem.AddProviderInfo()->SetProviderName(contentDetailsitem.GetProviderName());
    videoItem.AddProviderInfo()->SetAvailable(true);
    return videoItem;
}

} // namespace NAlice::NHollywood::NVideo
