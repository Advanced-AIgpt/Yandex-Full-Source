#include "video_database.h"

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/is_in.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood {

namespace {

// NOTE(dan-anastasev) Check DIALOG-6120 for additional information about the coefficient
constexpr double WATCHES_COUNT_COEF = 0.01;

constexpr TStringBuf EMBEDDER_CONFIG_PATH = "bigrams_v_20200101_config.json";
constexpr TStringBuf EMBEDDER_DATA_PATH = "bigrams_v_20200101.dssm";
constexpr TStringBuf VIDEO_DATA_PATH = "video_base.txt";

struct TItemIndexWithScore {
    size_t Index = 0;
    float Score = 0.f;
};

void FillStringArray(const NJson::TJsonValue& jsonArray, TVector<TString>& array) {
    for (const auto& jsonElement : jsonArray.GetArray()) {
        if (!jsonElement.IsString()) {
            continue;
        }
        array.push_back(jsonElement.GetString());
    }
}

TVideoDatabase::TItem ParseItemFromJson(const TString& jsonString, const TMovieInfoEmbedder& embedder) {
    const NJson::TJsonValue json = JsonFromString(jsonString);

    TVideoDatabase::TItem item;
    item.KinopoiskId = json["KinopoiskId"].GetUInteger();
    item.ContentType = json["ContentType"].GetString();
    item.MinAge = json["MinAge"].GetUInteger();
    item.Rating = json["Rating"].GetDouble();
    item.ReleaseYear = json["ReleaseYear"].GetInteger();
    item.LogWatchesCount = json["LogWatchesCount"].GetDouble();

    FillStringArray(json["Genres"], item.Genres);
    FillStringArray(json["Countries"], item.Countries);
    FillStringArray(json["Actors"], item.Actors);
    FillStringArray(json["Directors"], item.Directors);
    FillStringArray(json["Keywords"], item.Keywords);

    item.VideoItem = json["VideoItem"];

    const auto& title = json["Name"].GetString();
    // TODO: Add to json
    TStringBuf url = json["VideoItem"]["debug_info"]["web_page_url"].GetString();
    url.SkipPrefix("http://");
    const auto host = TString("http://") + url.Before('/');
    const auto path = TString("/") + url.After('/');

    item.Embedding = embedder.EmbedMovie(title, host, path);

    return item;
}

const TString& GetSlotType(const TSemanticFrame::TSlot& slot) {
    if (const auto& type = slot.GetType()) {
        return type;
    }
    return slot.GetTypedValue().GetType();
}

const TString& GetSlotValueString(const TSemanticFrame::TSlot& slot) {
    if (const auto& value = slot.GetValue()) {
        return value;
    }
    return slot.GetTypedValue().GetString();
}

bool CanUseSlotAsFeature(const TSemanticFrame::TSlot& slot, const TMaybe<TString>& filterValue,
                         const TStringBuf featureName) {
    const bool isRequested = slot.GetIsRequested() && !slot.GetIsFilled();
    return slot.GetName() == featureName && filterValue != GetSlotValueString(slot) && !isRequested;
}

void ConvertExactYearSlot(const TString& slotValue, TReleaseYearFeature& feature) {
    ui32 value = 0;
    if (TryFromString(slotValue, value)) {
        feature.ExactYear = value;
    }
}

void ConvertRelativeYearSlot(i32 relativeYear, const TClientInfoProto& clientInfo, TReleaseYearFeature& feature) {
    ui64 epoch = 0;
    if (!TryFromString(clientInfo.GetEpoch(), epoch)) {
        return;
    }

    const TInstant timeNow = TInstant::Seconds(epoch);
    try {
        const auto timezone = NDatetime::GetTimeZone(clientInfo.GetTimezone());
        const NDatetime::TSimpleTM civilTime = NDatetime::ToCivilTime(timeNow, timezone);

        feature.ExactYear = civilTime.RealYear() + relativeYear;
    } catch (const NDatetime::TInvalidTimezone& e) {
        // Don't use year slot in this case
    }
}

void ConvertYearAdjectiveSlot(const TString& slotValue, const TClientInfoProto& clientInfo,
                              TReleaseYearFeature& feature) {
    i32 from = 0, to = 0;
    TStringBuf buffer = slotValue;
    if (TryFromString(buffer.NextTok(':'), from)) {
        if (TryFromString(buffer, to)) {
            feature.YearsRange.ConstructInPlace(std::make_pair(from, to));
        } else {
            ConvertRelativeYearSlot(from, clientInfo, feature);
        }
    }
}

TReleaseYearFeature ConvertReleaseYearSlot(const TSemanticFrame::TSlot& slot, const TClientInfoProto& clientInfo) {
    TReleaseYearFeature feature;

    const TString& slotType = GetSlotType(slot);
    const TString& slotValue = GetSlotValueString(slot);

    if (slotType == "custom.year") {
        ConvertExactYearSlot(slotValue, feature);
    } else if (slotType == "custom.year_adjective") {
        ConvertYearAdjectiveSlot(slotValue, clientInfo, feature);
    }

    return feature;
}

bool ReleaseDateMatches(ui32 itemReleaseDate, const TReleaseYearFeature& expectedReleaseDateFeature) {
    if (!expectedReleaseDateFeature.Defined()) {
        return true;
    }

    if (expectedReleaseDateFeature.ExactYear) {
        return itemReleaseDate == *expectedReleaseDateFeature.ExactYear;
    }

    if (expectedReleaseDateFeature.YearsRange) {
        const auto& [from, to] = *expectedReleaseDateFeature.YearsRange;
        return from <= itemReleaseDate && itemReleaseDate <= to;
    }

    return false;
}

bool Matches(const TVideoDatabase::TItem& item, const TExpectedFeatures& expectedFeatures) {
    if (expectedFeatures.Genre && !IsIn(item.Genres, *expectedFeatures.Genre)) {
        return false;
    }

    if (expectedFeatures.Country && !IsIn(item.Countries, *expectedFeatures.Country)) {
        return false;
    }

    if (!ReleaseDateMatches(item.ReleaseYear, expectedFeatures.ReleaseYear)) {
        return false;
    }

    return true;
}

TVector<TItemIndexWithScore> OrderVideoItemsBySimilarityToQuery(const TVector<float>& queryEmbedding,
                                                                const TExpectedFeatures& expectedFeatures,
                                                                const TVector<TVideoDatabase::TItem>& items) {
    TVector<TItemIndexWithScore> result;
    for (size_t itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        const auto& item = items[itemIndex];
        if (!Matches(item, expectedFeatures)) {
            continue;
        }

        const float similarity = DotProduct(queryEmbedding.data(), item.Embedding.data(), item.Embedding.size());
        const float score = similarity + WATCHES_COUNT_COEF * item.LogWatchesCount;
        result.push_back({itemIndex, score});
    }

    Sort(result, [](const auto& lhs, const auto& rhs) {
        return lhs.Score > rhs.Score;
    });

    return result;
}

} // namespace

TExpectedFeatures ConvertSlotsToFeatures(const TSlots& slots, const TClientInfoProto& clientInfo,
                                         const TMaybe<TString>& filterValue) {
    TExpectedFeatures features;

    for (const auto& slot : slots) {
        if (CanUseSlotAsFeature(slot, filterValue, NVideoCommon::SLOT_GENRE)) {
            features.Genre = GetSlotValueString(slot);
        } else if (CanUseSlotAsFeature(slot, filterValue, NVideoCommon::SLOT_COUNTRY)) {
            features.Country = GetSlotValueString(slot);
        } else if (CanUseSlotAsFeature(slot, filterValue, NVideoCommon::SLOT_RELEASE_DATE)) {
            features.ReleaseYear = ConvertReleaseYearSlot(slot, clientInfo);
        } else if (CanUseSlotAsFeature(slot, filterValue, NVideoCommon::SLOT_ABOUT)) {
            features.About = GetSlotValueString(slot);
        }
    }

    return features;
}

void TVideoDatabase::LoadFromPath(const TFsPath& dirPath) {
    LoadFromPaths(dirPath / VIDEO_DATA_PATH, dirPath / EMBEDDER_DATA_PATH, dirPath / EMBEDDER_CONFIG_PATH);
}

void TVideoDatabase::LoadFromPaths(const TString& videoBasePath, const TString& embedderModelPath,
                                   const TString& embedderConfigPath)
{
    Embedder.Load(embedderModelPath, embedderConfigPath);

    TFileInput inputStream(videoBasePath);

    TString line;
    while (inputStream.ReadLine(line)) {
        auto item = ParseItemFromJson(line, Embedder);
        if (item.ContentType == "movie" || item.ContentType == "cartoon") {
            Items.push_back(std::move(item));
        }
    }

    Sort(Items, [](const auto& lhs, const auto& rhs) {
        return lhs.Rating > rhs.Rating;
    });
}

TVector<TVideoItem> TVideoDatabase::Recommend(const TExpectedFeatures& expectedFeatures,
                                              size_t gallerySize, size_t skipCount) const {
    if (expectedFeatures.About) {
        return RecommendWithTopSimilarity(expectedFeatures, gallerySize, skipCount);
    }
    return RecommendWithTopRating(expectedFeatures, gallerySize, skipCount);
}

TVector<TVideoItem> TVideoDatabase::RecommendWithTopRating(const TExpectedFeatures& expectedFeatures,
                                                           size_t gallerySize, size_t skipCount) const
{
    TVector<TVideoItem> result;
    size_t skippedCount = 0;
    for (const auto& item : Items) {
        if (!Matches(item, expectedFeatures)) {
            continue;
        }

        if (skippedCount < skipCount) {
            skippedCount += 1;
            continue;
        }

        result.push_back(JsonToProto<TVideoItem>(item.VideoItem));
        if (result.size() == gallerySize) {
            break;
        }
    }

    return result;
}

TVector<TVideoItem> TVideoDatabase::RecommendWithTopSimilarity(const TExpectedFeatures& expectedFeatures,
                                                               size_t gallerySize, size_t skipCount) const
{
    Y_ENSURE(expectedFeatures.About.Defined());

    const TVector<float> queryEmbedding = Embedder.EmbedQuery(*expectedFeatures.About);
    const TVector<TItemIndexWithScore> result =
        OrderVideoItemsBySimilarityToQuery(queryEmbedding, expectedFeatures, Items);

    TVector<TVideoItem> gallery;
    for (size_t i = skipCount; i < Min(gallerySize + skipCount, result.size()); ++i) {
        gallery.push_back(JsonToProto<TVideoItem>(Items[result[i].Index].VideoItem));
    }

    return gallery;
}

ui32 TVideoDatabase::RecommendableItemCount(const TExpectedFeatures& expectedFeatures) const {
    size_t recommendableCount = 0;
    for (const auto& item : Items) {
        if (Matches(item, expectedFeatures)) {
            recommendableCount += 1;
        }
    }

    return recommendableCount;
}

} // namespace NAlice::NHollywood
