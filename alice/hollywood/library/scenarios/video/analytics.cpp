#include "analytics.h"

#include <alice/library/video_common/defs.h>

using namespace NAlice::NVideo;
using namespace NAlice::NScenarios;
using namespace NAlice::NVideoCommon;

namespace NAlice::NHollywood::NVideo {

namespace { // private namespace

const THashMap<TStringBuf, TItem::ESource> SOURCE_NAME_TO_ENUM {
    {VIDEO_SOURCE_CAROUSEL, TItem::ESource::TItem_ESource_CAROUSEL},
    {VIDEO_SOURCE_ENTITY_SEARCH, TItem::ESource::TItem_ESource_ENTITY_SEARCH},
    {VIDEO_SOURCE_KINOPOISK_RECOMMENDATIONS, TItem::ESource::TItem_ESource_KINOPOISK_DUMMY_RECOMMENDATIONS},
    {VIDEO_SOURCE_WEB, TItem::ESource::TItem_ESource_WEB},
    {VIDEO_SOURCE_YAVIDEO_TOUCH, TItem::ESource::TItem_ESource_YAVIDEO_TOUCH},
    {VIDEO_SOURCE_YAVIDEO, TItem::ESource::TItem_ESource_YAVIDEO},
};

NAlice::NVideo::TItem GetAnalyticsItemFromOttVideoItem(const TOttVideoItem& item) {
    TItem analyticsItem;

    analyticsItem.SetUrl(item.GetEmbedUri());
    analyticsItem.SetName(item.GetTitle());
    analyticsItem.SetDescription(item.GetDescription());
    analyticsItem.SetKinopoiskId(item.GetProviderItemId());
    analyticsItem.SetSource(TItem::ESource::TItem_ESource_CAROUSEL);

    if (item.GetContentType() == "tv_show") {
        analyticsItem.SetType(TItem::EType::TItem_EType_TV_SHOW);
    } else if (item.GetContentType() == "movie") {
        analyticsItem.SetType(TItem::EType::TItem_EType_MOVIE);
    } else if (item.GetContentType() == "video") {
        analyticsItem.SetType(TItem::EType::TItem_EType_VIDEO);
    }

    return analyticsItem;
}
NAlice::NVideo::TItem GetAnalyticsItemFromWebVideoItem(const TWebVideoItem& item) {
    TItem analyticsItem;

    analyticsItem.SetUrl(item.GetEmbedUri());
    analyticsItem.SetName(item.GetTitle());
    analyticsItem.SetDescription(item.GetPlayerId());
    analyticsItem.SetSource(TItem::ESource::TItem_ESource_YAVIDEO);
    if (item.GetHosting().Contains("youtube")) {
        analyticsItem.SetYoutubeId(item.GetId());
    } else if (item.GetHosting().Contains("kinopoisk")) {
        analyticsItem.SetKinopoiskId(item.GetProviderItemId());
    } else {
        analyticsItem.SetYavideoUrl(item.GetProviderItemId());
    }

    if (item.GetContentType() == "tv_show") {
        analyticsItem.SetType(TItem::EType::TItem_EType_TV_SHOW);
    } else if (item.GetContentType() == "movie") {
        analyticsItem.SetType(TItem::EType::TItem_EType_MOVIE);
    } else if (item.GetContentType() == "video") {
        analyticsItem.SetType(TItem::EType::TItem_EType_VIDEO);
    }

    return analyticsItem;
}
NAlice::NVideo::TItem GetAnalyticsItemFromCollectionItem(const TCollectionItem& item) {
    TItem analyticsItem;

    analyticsItem.SetName(item.GetTitle());
    analyticsItem.SetDescription(item.GetSearchQuery());
    analyticsItem.SetUrl(item.GetEntref());
    analyticsItem.SetKinopoiskId(item.GetId());

    return analyticsItem;
}
NAlice::NVideo::TItem GetAnalyticsItemFromPersonItem(const TPersonItem& item) {
    TItem analyticsItem;

    analyticsItem.SetName(item.GetName());
    analyticsItem.SetDescription(item.GetSubtitle());
    analyticsItem.SetUrl(item.GetEntref());
    analyticsItem.SetKinopoiskId(item.GetKpId());

    return analyticsItem;
}

NAlice::NVideo::TItem GetAnalyticsItemFromVideoItem(const TVideoItem& videoItem) {
    TItem analyticsItem;

    analyticsItem.SetUrl(videoItem.GetDebugInfo().GetWebPageUrl());
    analyticsItem.SetName(videoItem.GetName());
    analyticsItem.SetDescription(videoItem.GetDescription());

    {
        const auto& providerName = videoItem.GetProviderName();
        const auto& providerItemId = videoItem.GetProviderItemId();

        if (providerName == PROVIDER_KINOPOISK) {
            analyticsItem.SetKinopoiskId(providerItemId);
        } else if (providerName == PROVIDER_YOUTUBE) {
            analyticsItem.SetYoutubeId(providerItemId);
        } else if (providerName == PROVIDER_YAVIDEO || providerName == PROVIDER_YAVIDEO_PROXY) {
            analyticsItem.SetYavideoUrl(providerItemId);
        }
    }
    if (const auto* sourceEnum = SOURCE_NAME_TO_ENUM.FindPtr(videoItem.GetSource())) {
        analyticsItem.SetSource(*sourceEnum);
    }

    if (videoItem.GetType() == "tv_show") {
        analyticsItem.SetType(TItem::EType::TItem_EType_TV_SHOW);
    } else if (videoItem.GetType() == "movie") {
        analyticsItem.SetType(TItem::EType::TItem_EType_MOVIE);
    } else if (videoItem.GetType() == "video") {
        analyticsItem.SetType(TItem::EType::TItem_EType_VIDEO);
    }

    return analyticsItem;
}

} // namespace


TAnalyticsInfo::TObject GetAnalyticsObjectForDescription(const TVideoItem& item, bool withPayScreen) {
    TAnalyticsInfo::TObject object;
    object.SetId("1");
    object.SetName("description");
    object.SetHumanReadable("Film or serial description screen");

    TDescriptionScreen descriptionScreen;
    *descriptionScreen.MutableItem() = GetAnalyticsItemFromVideoItem(item);
    descriptionScreen.SetWithPayScreen(withPayScreen);
    *object.MutableVideoDescriptionScreen() = descriptionScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectForGallery(const TVector<TVideoItem>& items) {
    TAnalyticsInfo::TObject object;
    object.SetId("2");
    object.SetName("gallery");
    object.SetHumanReadable("Film, serial or video search gallery");

    TSearchGalleryScreen searchGalleryScreen;
    for (const TVideoItem& item : items) {
        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromVideoItem(item);
    }
    *object.MutableVideoSearchGalleryScreen() = searchGalleryScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectForTvSearch(const TTvSearchResultData& data) {
    TAnalyticsInfo::TObject object;
    object.SetId("2");
    object.SetName("gallery");
    object.SetHumanReadable("Film, serial or video search gallery");

    TSearchGalleryScreen searchGalleryScreen;
    for (const auto& carousel: data.GetGalleries()) {
        switch (carousel.GetCarouselCase()) {
        case TTvSearchCarouselWrapper::kBasicCarousel:
        {
            const auto& basicCarousel = carousel.GetBasicCarousel();
            for (const NTv::TCarouselItemWrapper& itemWrapper : basicCarousel.GetItems()) {
                switch (itemWrapper.GetItemCase()) {
                    case NTv::TCarouselItemWrapper::kPersonItem:
                        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromPersonItem(itemWrapper.GetPersonItem());
                        break;
                    case NTv::TCarouselItemWrapper::kCollectionItem:
                        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromCollectionItem(itemWrapper.GetCollectionItem());
                        break;
                    case NTv::TCarouselItemWrapper::kVideoItem:
                        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromOttVideoItem(itemWrapper.GetVideoItem());
                        break;
                    case NTv::TCarouselItemWrapper::kSearchVideoItem:
                        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromWebVideoItem(itemWrapper.GetSearchVideoItem());
                        break;
                    case NTv::TCarouselItemWrapper::kMusicItem:
                    case NTv::TCarouselItemWrapper::kRecentApplicationsItem:
                    case NTv::TCarouselItemWrapper::ITEM_NOT_SET:
                        break;
                }
            }
            break;
        }
        case TTvSearchCarouselWrapper::CAROUSEL_NOT_SET:
            break;
        }
    }
    *object.MutableVideoSearchGalleryScreen() = searchGalleryScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectForSeasonGallery(const TVideoItem& tvShowItem,
                                                           const TVector<TVideoItem>& items,
                                                           ui32 seasonNumber) {
    TAnalyticsInfo::TObject object;
    object.SetId("3");
    object.SetName("season_gallery");
    object.SetHumanReadable("Gallery with episodes");

    TSeasonGalleryScreen seasonGalleryScreen;
    *seasonGalleryScreen.MutableParent() = GetAnalyticsItemFromVideoItem(tvShowItem);
    seasonGalleryScreen.SetSeasonNumber(seasonNumber);
    for (const auto& item : items) {
        seasonGalleryScreen.AddEpisodes(item.GetName());
    }
    *object.MutableVideoSeasonGalleryScreen() = seasonGalleryScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectCurrentlyPlayingVideo(const TVideoItem& item) {
    TAnalyticsInfo::TObject object;
    object.SetId("4");
    object.SetName("currently_playing_video");
    object.SetHumanReadable("Currently playing video");

    TCurrentlyPlayingVideo currentlyPlayingVideo;
    *currentlyPlayingVideo.MutableItem() = GetAnalyticsItemFromVideoItem(item);
    *object.MutableCurrentlyPlayingVideo() = currentlyPlayingVideo;

    return object;
}

} // namespace NAlice::NHollywood::NVideo