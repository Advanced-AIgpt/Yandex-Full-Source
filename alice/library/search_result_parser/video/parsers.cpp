#include "parsers.h"

#include <alice/library/search_result_parser/video/matcher_util.h>
#include <alice/library/search_result_parser/video/parser_util.h>

using namespace NJson;
namespace NAlice::NHollywood::NVideo::SearchResultParser {

// forward declarations of *private* functions
template<typename SomeValue>
static TOttVideoItem MakeOttVideoItem(const SomeValue& jsonItem, const TStringBuf cinemaField = "cinemas", bool strictCinemaData = true);

template<typename SomeValue>
static TWebVideoItem MakeWebVideoItem(const SomeValue& jsonItem, const TString& searchReqId, TRTLogger& logger);

template<typename SomeValue>
static TPersonItem MakePersonItem(const SomeValue& jsonItem, TRTLogger& logger);

template<typename SomeValue>
static TCollectionItem MakeCollectionItem(const SomeValue& item, TRTLogger& logger);

template<typename SomeValue>
static TVHLicenceInfo MakeTVHLicenseInfo(const SomeValue& vhLicenseInfo);
// end of declarations

template<typename SomeValue>
TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfoImpl(const SomeValue& entityData, TRTLogger& logger, std::function<bool(const TOttVideoItem&)> checker) {
    if (entityData.Has("base_info")) {
        const auto& baseInfo = entityData["base_info"];

        NTv::TCarouselItemWrapper wrapper;
        if (baseInfo["type"].GetString() == "Hum" && ValidPerson(baseInfo)) {
            *wrapper.MutablePersonItem() = MakePersonItem(baseInfo, logger);
            return wrapper;
        } else if (baseInfo["type"].GetString() == "Film") {
            auto ottVideo = MakeOttVideoItem(baseInfo);
            if (auto cinemaData = MakeCinemaData(entityData["rich_info"]["cinema_data"]); !cinemaData.empty()) {
                *ottVideo.MutableCinemas() = std::move(cinemaData);
            }
            if (checker(ottVideo)) {
                *wrapper.MutableVideoItem() = std::move(ottVideo);
                return wrapper;
            }
        }
    }
    return Nothing();
}

template<typename SomeValue>
TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfo(const SomeValue& entityData, TRTLogger& logger) {
    return ParseBaseInfoImpl(entityData, logger, IsUsefulKpItem);
}
template TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfo(const NJson::TJsonValue& response, TRTLogger& logger);
template TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfo(const TProtoAdapter& response, TRTLogger& logger);

template<typename SomeValue>
TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfoWide(const SomeValue& entityData, TRTLogger& logger) {
    return ParseBaseInfoImpl(entityData, logger, IsUsefulOttItem);
}

template<typename SomeValue>
TTvSearchResultData ParseImpl(const SomeValue& response, const TStringBuf entityKey, TRTLogger& logger, bool useHalfPiratesFromBaseInfo) {
    TTvSearchResultData parsedData;

    if (auto parent = ParseParentCollectionObjects(response[entityKey], logger)) {
        LOG_INFO(logger) << "Added parentCollection of " << parent->ItemsSize();
        auto& parentGallery = *parsedData.AddGalleries();
        *parentGallery.MutableBasicCarousel() = std::move(*parent.Get());
    }
    auto& firstGallery = *parsedData.AddGalleries();
    auto baseInfoParser = useHalfPiratesFromBaseInfo ? ParseBaseInfoWide<SomeValue> : ParseBaseInfo<SomeValue>;
    if (auto baseInfo = baseInfoParser(response[entityKey], logger)) {
        *firstGallery.MutableBasicCarousel()->AddItems() = std::move(*baseInfo);
        LOG_INFO(logger) << "Added baseInfo to parsedData";
    }

    if (auto clips = ParseClips(response, logger); !clips.empty()) {
        LOG_INFO(logger) << "Added " << clips.size() << " clips";
        for (auto& clip : clips) {
            *firstGallery.MutableBasicCarousel()->AddItems() = std::move(clip);
        }
    }
    if (auto relatedObjects = ParseRelatedObjects(response[entityKey], logger)) {
        LOG_INFO(logger) << "Added " << relatedObjects->size() << " galleries:";
        for (auto i = 0; i < relatedObjects->size(); ++i) {
            LOG_DEBUG(logger) << "Gallery " << i << ": " << (*relatedObjects)[i].GetTitle() << " with " << (*relatedObjects)[i].ItemsSize() << " items";
            *parsedData.AddGalleries()->MutableBasicCarousel() = std::move((*relatedObjects)[i]);
        }
    }
    return parsedData;
}

TTvSearchResultData ParseJsonResponse(const NJson::TJsonValue& response, TRTLogger& logger, bool useHalfPiratesFromBaseInfo) {
    return ParseImpl(response, "entity_data", logger, useHalfPiratesFromBaseInfo);
}

TTvSearchResultData ParseProtoResponse(const TProtoAdapter& response, TRTLogger& logger, bool useHalfPiratesFromBaseInfo) {
    return ParseImpl(response, "data", logger, useHalfPiratesFromBaseInfo);
}

template<typename SomeValue>
TOttVideoItem MakeOttVideoItem(const SomeValue& jsonItem, const TStringBuf cinemaField, bool strictCinemaData) {
    TOttVideoItem videoItem;

    bool hasLegal = jsonItem.Has("legal") && jsonItem["legal"].Has("vh_licenses");
    if (hasLegal) {
        *videoItem.MutableVhLicences() = MakeTVHLicenseInfo(jsonItem["legal"]["vh_licenses"]);
    }

    // important uuid filling
    if (hasLegal && !jsonItem["legal"]["vh_licenses"]["uuid"].GetString().Empty()) {
        videoItem.SetProviderItemId(jsonItem["legal"]["vh_licenses"]["uuid"].GetString());
    } else if (jsonItem.Has("vh_uuid")) {
        videoItem.SetProviderItemId(jsonItem["vh_uuid"].GetString());
    }

    videoItem.MutableMiscIds()->SetOntoId(jsonItem["id"].GetString());
    videoItem.SetTitle(jsonItem["title"].GetString());

    if (jsonItem.Has("wsubtype")) {
        for (const auto& subtype : jsonItem["wsubtype"].GetArray()) {
            if (subtype.GetString().Contains("Film")) {
                videoItem.SetContentType("movie");
            } else if (subtype.GetString().Contains("Series")) {
                videoItem.SetContentType("tv_show");
            }
        }
    }
    if (jsonItem.Has(cinemaField)) {
        *videoItem.MutableCinemas() = MakeCinemaData(jsonItem, cinemaField, strictCinemaData);
    }

    if (auto maybePoster = BuildImage(jsonItem["image"]["original"].GetString(), posterImageResolutions)) {
        *videoItem.MutablePoster() = std::move(*maybePoster);
    }

    videoItem.SetDescription(jsonItem["description"].GetString());
    if (jsonItem.Has("rating")) {
        if (auto rating = TryGetRating(jsonItem)) {
            videoItem.SetRating(*rating);
        }
    }
    // TODO(kolchanovs): Why not ReleaseYear ?!
    videoItem.SetReleaseDate(jsonItem["release_year"].GetUInteger());
    videoItem.SetHintDescription(jsonItem["hint_description"].GetString());
    videoItem.SetEntref(jsonItem["entref"].GetString());
    videoItem.SetAgeLimit(jsonItem["age_limit"].GetString());
    videoItem.SetSearchQuery(jsonItem["search_request"].GetString());

    // TODO(kolchanovs) : What about "duration" ?

    return videoItem;
}

template<typename SomeValue>
TWebVideoItem MakeWebVideoItem(const SomeValue& jsonItem, const TString& searchReqId, TRTLogger& logger) {
    TWebVideoItem item;
    if (!jsonItem.Has("onto_id")) {
        TString ontoId = jsonItem["ontoId"].GetString();
        item.SetId(ontoId);
        item.MutableMiscIds()->SetOntoId(ontoId);
    } else if (jsonItem["VisibleHost"].GetString().Contains("youtube")) {
        if (const auto& youtubeId = TryFindYoutubeUri(jsonItem["url"].GetString())) {
            item.SetId(*youtubeId);
            item.SetProviderItemId(*youtubeId);
        }
    }
    item.SetContentType("video");
    item.SetTitle(jsonItem["title"].GetString());
    item.SetPlayerId(jsonItem["PlayerId"].GetString());
    if (jsonItem.Has("vh_uuid")) {
        item.SetProviderItemId(jsonItem["vh_uuid"].GetString());
    }
    if (auto maybeThumbnail = BuildImage("https:" + jsonItem["thmb_href"].GetString() + "/orig", thumbnailImageResolutions)) {
        *item.MutableThumbnail() = std::move(*maybeThumbnail);
    }
    //item.Poster();
    item.SetHosting(jsonItem["VisibleHost"].GetString());
    if (const auto& EmbedUri = FindEmbedUrl(jsonItem["players"]["autoplay"]["html"].GetStringRobust())) {
        item.SetEmbedUri(FixSchema(*EmbedUri));
    }
    item.SetIsCommercialVideo(false);
    if (jsonItem.Has("mtime")) {
        try {
            auto ts = TInstant::ParseIso8601(jsonItem["mtime"].GetString());
            item.SetReleaseDate(ts.Seconds());
        } catch (const yexception &e) {
            LOG_WARN(logger) << "clip object has invalid mtime: " << jsonItem["mtime"].GetString();
        }
    }
    item.SetDuration(jsonItem["duration"].GetUInteger());
    item.SetReqId(searchReqId);
    return item;
}

template<typename SomeValue>
TPersonItem MakePersonItem(const SomeValue& jsonItem, TRTLogger& logger) {
    TPersonItem item;
    item.SetName(jsonItem["name"].GetString());
    item.SetDescription(jsonItem.Has("description") ? jsonItem["description"].GetString() : jsonItem["subtitle"].GetString());
    if (jsonItem.Has("subtitle")) item.SetSubtitle(jsonItem["subtitle"].GetString());
    item.SetEntref(jsonItem["entref"].GetString());
    item.SetSearchQuery(jsonItem["search_request"].GetString());
    if (jsonItem.Has("ids") && jsonItem["ids"].Has("kinopoisk")) {
        auto raw_kp = jsonItem["ids"]["kinopoisk"].GetString();
        if (auto kpId = TryFindPersonKpId(raw_kp)) {
            item.SetKpId(*kpId);
        } else {
            LOG_ERR(logger) << "Can't extract KpId from: " << raw_kp;
        }
    } else LOG_ERR(logger) << "No KpId for person " << jsonItem["id"].GetString();

    if (jsonItem.Has("image") && jsonItem["image"].Has("original")) {
        if (auto maybeImage = BuildImage(jsonItem["image"]["original"].GetString(), posterImageResolutions)) {
            *item.MutableImage() = std::move(*maybeImage);
        }
    }

    return item;
}

template<typename SomeValue>
TCollectionItem MakeCollectionItem(const SomeValue& jsonItem, TRTLogger& logger) {
    TCollectionItem item;
    item.SetId(jsonItem["id"].GetString());
    item.SetTitle(jsonItem["title"].GetString());
    item.SetEntref(jsonItem["entref"].GetString());
    item.SetSearchQuery(jsonItem["search_request"].GetString());

    typename SomeValue::TArray images;
    if (jsonItem.Has("collection_images")) {
        images = jsonItem["collection_images"].GetArray();
    } else if (jsonItem.Has("image")) {
        images.emplace_back(jsonItem["image"]);
    }
    for (auto& image: images) {
        if (auto maybeImage = BuildImage(image["original"].GetString(), thumbnailImageResolutions)) {
            *item.MutableImages()->Add() = std::move(*maybeImage);
        } else {
            LOG_DEBUG(logger) << "Unprocessable image: " << image.GetStringRobust();
        }
    }
    return item;
}

template<typename SomeValue>
TVHLicenceInfo MakeTVHLicenseInfo(const SomeValue& vhLicenses) {
    TVHLicenceInfo result;
    result.SetAvod(vhLicenses.Has("avod") ? 1 : 0);
    if (vhLicenses.Has("tvod")) {
        const auto& tvod = vhLicenses["tvod"];
        result.SetUserHasTvod(tvod.Has("TVOD") ? tvod["TVOD"].GetBoolean() : 0);
        result.SetTvod(tvod["price"].GetInteger());
    } else {
        result.SetUserHasTvod(0);
        result.SetTvod(0);
    }
    if (vhLicenses.Has("svod") && vhLicenses["svod"].Has("subscriptions")) {
        for (const auto& subscription : vhLicenses["svod"]["subscriptions"].GetArray()) {
            result.AddSvod(subscription.GetString());
        }
    }
    if (vhLicenses.Has("est")) {
        const auto& est = vhLicenses["est"];
        result.SetUserHasEst(est.Has("EST") ? est["EST"].GetInteger() : 0);
        if (est.Has("price")) result.SetEst(est["price"].GetInteger());
    } else {
        result.SetUserHasEst(0);
        result.SetEst(0);
    }
    if (vhLicenses.Has("content_type")) result.SetContentType(vhLicenses["content_type"].GetString());
    return result;
}

template <typename SomeValue>
TMaybe<NProtoBuf::RepeatedPtrField<NTv::TCarousel>> ParseRelatedObjects(const SomeValue& entityData, TRTLogger& logger) {
    if (entityData.Has("related_object")) {
        const static TVector<TString> CarouselsSequence = {"team", "assoc", "collections"};
        NProtoBuf::RepeatedPtrField<NTv::TCarousel> carousels;

        for (const auto& galleryName: CarouselsSequence) {
            for (const auto& galeryJson: entityData["related_object"].GetArray()) {
                if (galeryJson["type"].GetString() == galleryName) {
                    auto& carousel = *carousels.Add();
                    carousel.SetId(galeryJson["id"].GetString());
                    carousel.SetTitle(galeryJson["list_name"].GetString());

                    for (const auto& relatedObject : galeryJson["object"].GetArray()) {
                        const auto& relatedObjectType = relatedObject["type"].GetString();
                        if (relatedObjectType == "Film") {
                            if (auto video = MakeOttVideoItem(relatedObject); IsUsefulOttItem(video)) {
                                *carousel.AddItems()->MutableVideoItem() = std::move(video);
                            }
                        } else if (relatedObjectType == "Hum") {
                            *carousel.AddItems()->MutablePersonItem() = MakePersonItem(relatedObject, logger);
                        } else if (relatedObjectType == "List") {
                            *carousel.AddItems()->MutableCollectionItem() = MakeCollectionItem(relatedObject, logger);
                        }
                    }
                }
            }
        }
        return carousels;
    }
    return Nothing();
}
template TMaybe<NProtoBuf::RepeatedPtrField<NTv::TCarousel>> ParseRelatedObjects(const NJson::TJsonValue& response, TRTLogger& logger);
template TMaybe<NProtoBuf::RepeatedPtrField<NTv::TCarousel>> ParseRelatedObjects(const TProtoAdapter& response, TRTLogger& logger);

template<typename SomeValue>
TMaybe<NTv::TCarousel> ParseParentCollectionObjects(const SomeValue& entityData, TRTLogger&) {
    bool responseHasParentCollection = entityData.Has("parent_collection")
        && entityData["parent_collection"].Has("object");

    if (!responseHasParentCollection) {
        return Nothing();
    }

    auto carousel = NTv::TCarousel();
    for (const auto& object : entityData["parent_collection"]["object"].GetArray()) {
        auto videoItem = MakeOttVideoItem(object, "film_offers", false);
        if (!IsUsefulOttItem(videoItem)) {
            continue;
        }
        videoItem.SetContentType("association");
        const auto& item = carousel.AddItems();
        *item->MutableVideoItem() = std::move(videoItem);
    }
    return carousel;
}
template TMaybe<NTv::TCarousel> ParseParentCollectionObjects(const NJson::TJsonValue& entityData, TRTLogger&);
template TMaybe<NTv::TCarousel> ParseParentCollectionObjects(const TProtoAdapter& entityData, TRTLogger&);

template<typename SomeValue>
NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper> ParseClips(const SomeValue& response, TRTLogger& logger) {
    NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper> items;
    for (const auto& clip: response["clips"].GetArray()) {
        *items.Add()->MutableSearchVideoItem() = std::move(MakeWebVideoItem(clip, response["reqid"].GetString(), logger));
    }
    return items;
}
template NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper> ParseClips(const NJson::TJsonValue& response, TRTLogger& logger);
template NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper> ParseClips(const TProtoAdapter& response, TRTLogger& logger);
} // namespace NAlice::NHollywood::NVideo::SearchResultParser
