#include "video_item.h"

#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/analytics/scenarios/video/video.pb.h>

#include <library/cpp/scheme/scheme.h>

using namespace NAlice::NVideo;
using namespace NAlice::NScenarios;
using namespace NAlice::NVideoCommon;

namespace {

const THashMap<TStringBuf, TItem::ESource> SOURCE_NAME_TO_ENUM {
    {VIDEO_SOURCE_CAROUSEL, TItem::ESource::TItem_ESource_CAROUSEL},
    {VIDEO_SOURCE_ENTITY_SEARCH, TItem::ESource::TItem_ESource_ENTITY_SEARCH},
    {VIDEO_SOURCE_KINOPOISK_RECOMMENDATIONS, TItem::ESource::TItem_ESource_KINOPOISK_DUMMY_RECOMMENDATIONS},
    {VIDEO_SOURCE_WEB, TItem::ESource::TItem_ESource_WEB},
    {VIDEO_SOURCE_YAVIDEO_TOUCH, TItem::ESource::TItem_ESource_YAVIDEO_TOUCH},
    {VIDEO_SOURCE_YAVIDEO, TItem::ESource::TItem_ESource_YAVIDEO},
};

TItem GetAnalyticsItemFromVideoItem(const NSc::TValue& videoItem) {
    TItem analyticsItem;

    analyticsItem.SetUrl(ToString(videoItem["debug_info"]["web_page_url"].GetString()));
    analyticsItem.SetName(ToString(videoItem["name"].GetString()));
    analyticsItem.SetDescription(ToString(videoItem["description"].GetString()));
    if (videoItem["provider_name"] == PROVIDER_KINOPOISK) {
        analyticsItem.SetKinopoiskId(ToString(videoItem["provider_item_id"].GetString()));
    } else if (videoItem["provider_name"] == PROVIDER_YOUTUBE) {
        analyticsItem.SetYoutubeId(ToString(videoItem["provider_item_id"].GetString()));
    } else if (videoItem["provider_name"] == PROVIDER_YAVIDEO ||
               videoItem["provider_name"] == PROVIDER_YAVIDEO_PROXY) {
        analyticsItem.SetYavideoUrl(ToString(videoItem["provider_item_id"].GetString()));
    }
    if (const auto* sourceEnum = SOURCE_NAME_TO_ENUM.FindPtr(ToString(videoItem["source"].GetString()))) {
        analyticsItem.SetSource(*sourceEnum);
    }

    if (videoItem["type"] == "tv_show") {
        analyticsItem.SetType(TItem::EType::TItem_EType_TV_SHOW);
    } else if (videoItem["type"] == "movie") {
        analyticsItem.SetType(TItem::EType::TItem_EType_MOVIE);
    } else if (videoItem["type"] == "video") {
        analyticsItem.SetType(TItem::EType::TItem_EType_VIDEO);
    }

    return analyticsItem;
}

} // namespace

namespace NAlice::NMegamind {

TAnalyticsInfo::TObject GetAnalyticsObjectForDescription(const NSc::TValue& item, bool withPayScreen) {
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

TAnalyticsInfo::TObject GetAnalyticsObjectForGallery(const NSc::TValue& items) {
    TAnalyticsInfo::TObject object;
    object.SetId("2");
    object.SetName("gallery");
    object.SetHumanReadable("Film, serial or video search gallery");

    TSearchGalleryScreen searchGalleryScreen;
    for (const auto& item : items.GetArray()) {
        *searchGalleryScreen.AddItems() = GetAnalyticsItemFromVideoItem(item);
    }
    *object.MutableVideoSearchGalleryScreen() = searchGalleryScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectForSeasonGallery(const NSc::TValue& tvShowItem,
                                                           const NSc::TValue& items,
                                                           ui32 seasonNumber) {
    TAnalyticsInfo::TObject object;
    object.SetId("3");
    object.SetName("season_gallery");
    object.SetHumanReadable("Gallery with episodes");

    TSeasonGalleryScreen seasonGalleryScreen;
    *seasonGalleryScreen.MutableParent() = GetAnalyticsItemFromVideoItem(tvShowItem);
    seasonGalleryScreen.SetSeasonNumber(seasonNumber);
    for (const auto& item : items.GetArray()) {
        seasonGalleryScreen.AddEpisodes(ToString(item["name"].GetString()));
    }
    *object.MutableVideoSeasonGalleryScreen() = seasonGalleryScreen;

    return object;
}

TAnalyticsInfo::TObject GetAnalyticsObjectCurrentlyPlayingVideo(const NSc::TValue& item) {
    TAnalyticsInfo::TObject object;
    object.SetId("4");
    object.SetName("currently_playing_video");
    object.SetHumanReadable("Currently playing video");

    TCurrentlyPlayingVideo currentlyPlayingVideo;
    *currentlyPlayingVideo.MutableItem() = GetAnalyticsItemFromVideoItem(item);
    *object.MutableCurrentlyPlayingVideo() = currentlyPlayingVideo;

    return object;
}

} // namespace NAlice::NMegamind
