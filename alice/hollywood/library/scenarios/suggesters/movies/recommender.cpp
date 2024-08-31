#include "recommender.h"

#include <alice/hollywood/library/scenarios/suggesters/common/recommender_utils.h>

#include <alice/library/video_common/age_restriction.h>

#include <library/cpp/resource/resource.h>

#include <util/stream/file.h>
#include <util/stream/str.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "suggestions.json";

template <typename TContainerAppender>
void ParseStringArray(const NJson::TJsonValue& jsonArray, TContainerAppender appender) {
    for (const auto& jsonElement : jsonArray.GetArray()) {
        if (!jsonElement.IsString()) {
            continue;
        }
        appender(jsonElement.GetString());
    }
}

TMovieRecommender::TItem ParseJsonItem(const NJson::TJsonValue& jsonItem) {
    TMovieRecommender::TItem item;

    item.Name = jsonItem["name"].GetString();
    item.ItemId = jsonItem["item_id"].GetString();
    item.IsPornoGenre = NVideoCommon::IsPornoGenre(jsonItem["genre"].GetString());
    item.MinAge = jsonItem["min_age"].GetInteger();
    item.ContentType = jsonItem["content_type"].GetString();

    ParseStringArray(jsonItem["text"], [&item](const auto& text) { item.Texts.push_back(text); });
    ParseStringArray(jsonItem["bass_genres"], [&item](const auto& text) { item.BassGenres.insert(text); });

    if (jsonItem.Has("persuading_text") && jsonItem["persuading_text"].IsDefined()) {
        item.PersuadingText = jsonItem["persuading_text"].GetString();
    }

    item.VideoItem = JsonToProto<TVideoItem>(jsonItem["video_item"]);

    return item;
}

bool IsAgeRestrictionPassed(const TMovieRecommender::TItem& item, EContentRestrictionLevel restrictionLevel) {
    NVideoCommon::TAgeRestrictionCheckerParams ageRestrictionCheckerParams;
    ageRestrictionCheckerParams.IsPornoGenre = item.IsPornoGenre;
    ageRestrictionCheckerParams.MinAge = item.MinAge;
    ageRestrictionCheckerParams.RestrictionLevel = restrictionLevel;

    return NVideoCommon::PassesAgeRestriction(ageRestrictionCheckerParams);
}

bool AreRestrictionsPassed(const TMovieRecommender::TItem& item, const TMovieRecommender::TRestrictions& restrictions) {
    return IsAgeRestrictionPassed(item, restrictions.Age)
        && !restrictions.ItemIds.contains(item.ItemId)
        && (!restrictions.ContentType || item.ContentType == restrictions.ContentType)
        && (!restrictions.Genre || item.BassGenres.contains(*restrictions.Genre));
}

} // namespace

const TString& TMovieRecommender::TItem::GetText(bool isPersuasionStep, IRng& rng) const {
    if (isPersuasionStep && PersuadingText) {
        return *PersuadingText;
    }

    const auto textIndex = rng.RandomInteger(Texts.size());
    return Texts[textIndex];
}

const TMovieRecommender::TItem* TMovieRecommender::Recommend(const TRestrictions& restrictions, IRng& rng) const {
    return ::NAlice::NHollywood::Recommend(Items, rng, [&restrictions](const auto& item) {
        return AreRestrictionsPassed(item, restrictions);
    });
}

const TMovieRecommender::TItem* TMovieRecommender::GetItemById(const TString& itemId) const {
    for (const TItem& item : Items) {
        if (item.ItemId == itemId) {
            return &item;
        }
    }

    return nullptr;
}

void TMovieRecommender::LoadFromPath(const TFsPath& dirPath) {
    TFileInput inputStream(dirPath / DATA_PATH);
    const NJson::TJsonValue data = JsonFromString(inputStream.ReadAll());

    LoadFromJson(data);
}

void TMovieRecommender::LoadFromJson(const NJson::TJsonValue& data) {
    for (const auto& recommendationsJson : data.GetArray()) {
        Items.push_back(ParseJsonItem(recommendationsJson));
    }
}

} // namespace NAlice::NHollywood
