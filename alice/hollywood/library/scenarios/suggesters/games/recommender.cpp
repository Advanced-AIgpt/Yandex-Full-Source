#include "recommender.h"

#include <library/cpp/resource/resource.h>

#include <util/stream/file.h>
#include <util/stream/str.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "game_suggestions.json";

TGameRecommender::TItem ParseJsonItem(const NJson::TJsonValue& jsonItem) {
    TGameRecommender::TItem item;

    item.Name = jsonItem["name"].GetString();
    item.ItemId = jsonItem["item_id"].GetString();

    for (const auto& jsonElement : jsonItem["responses"].GetArray()) {
        if (!jsonElement["text"].IsString()) {
            continue;
        }

        const TString& text = jsonElement["text"].GetString();
        if (!jsonElement["voice"].IsString()) {
            item.Responses.push_back({text, text});
        } else {
            const TString& voice = jsonElement["voice"].GetString();
            item.Responses.push_back({text, voice});
        }
    }

    return item;
}

} // namespace

const TResponseNlg& TGameRecommender::TItem::GetResponse(IRng& rng) const {
    const auto responseIndex = rng.RandomInteger(Responses.size());
    return Responses[responseIndex];
}

const TGameRecommender::TItem* TGameRecommender::Recommend(const TRestrictions& restrictions, IRng& rng) const {
    return ::NAlice::NHollywood::Recommend(Items, rng, [&restrictions](const auto& item) {
        return !restrictions.ItemIds.contains(item.ItemId);
    });
}

void TGameRecommender::LoadFromPath(const TFsPath& dirPath) {
    TFileInput inputStream(dirPath / DATA_PATH);
    const NJson::TJsonValue data = JsonFromString(inputStream.ReadAll());

    LoadFromJson(data);
}

void TGameRecommender::LoadFromJson(const NJson::TJsonValue& data) {
    for (const auto& recommendationsJson : data.GetArray()) {
        Items.push_back(ParseJsonItem(recommendationsJson));
    }
}

} // namespace NAlice::NHollywood
