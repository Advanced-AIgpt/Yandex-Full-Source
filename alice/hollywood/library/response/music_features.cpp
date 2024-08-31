#include "music_features.h"

#include <alice/library/factors/dcg.h>
#include <alice/library/response_similarity/response_similarity.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywood {

using namespace NResponseSimilarity;

namespace {

const NJson::TJsonValue* GetMapValue(const NJson::TJsonValue& dict, const TStringBuf key) {
    return dict.GetMap().FindPtr(key);
}

ui32 ComputeSimilarity(TSimilarity& similarity, const TStringBuf searchText, const NJson::TJsonValue& dict,
                       const TStringBuf path) {
    if (const NJson::TJsonValue* value = dict.GetValueByPath(path)) {
        const TString& response = value->GetString();
        similarity = CalculateResponseItemSimilarity(searchText, response, ELanguage::LANG_RUS);
        return 1;
    }
    return 0;
}

ui32 ComputeAggregatedSimilarity(TSimilarity& similarity, const TStringBuf searchText, const NJson::TJsonValue& dict,
                                 const TStringBuf key, const TStringBuf elementKey) {
    if (const auto* elems = GetMapValue(dict, key); elems && elems->IsArray()) {
        TVector<TSimilarity> similarities(Reserve(elems->GetArray().size()));
        for (const auto& elem : elems->GetArray()) {
            if (const auto* value = GetMapValue(elem, elementKey)) {
                similarities.emplace_back(
                    CalculateResponseItemSimilarity(searchText, value->GetString(), ELanguage::LANG_RUS));
            }
        }
        similarity = AggregateSimilarity(similarities);
        return similarities.size();
    }
    return 0;
}

} // namespace

ui32 FillMusicFeaturesProto(const TStringBuf searchText, const NJson::TJsonValue& searchResult, bool isPlayerCommand,
                            NScenarios::TMusicFeatures& features) {
    auto& resultTrackNameSimilarity = *(features.MutableResult()->MutableTrackNameSimilarity());
    auto& resultAlbumNameSimilarity = *(features.MutableResult()->MutableAlbumNameSimilarity());
    auto& resultArtistNameSimilarity = *(features.MutableResult()->MutableArtistNameSimilarity());

    auto& wizardTitleSimilarity = *(features.MutableWizard()->MutableTitleSimilarity());
    auto& wizardTrackNameSimilarity = *(features.MutableWizard()->MutableTrackNameSimilarity());
    auto& wizardAlbumNameSimilarity = *(features.MutableWizard()->MutableAlbumNameSimilarity());
    auto& wizardArtistNameSimilarity = *(features.MutableWizard()->MutableArtistNameSimilarity());
    auto& wizardTrackLyricsSimilarity = *(features.MutableWizard()->MutableTrackLyricsSimilarity());

    auto& documentsTitleSimilarity = *(features.MutableDocuments()->MutableTitleSimilarity());
    auto& documentsSnippetSimilarity = *(features.MutableDocuments()->MutableSnippetSimilarity());

    features.SetIsPlayerCommand(isPlayerCommand);

    ui32 normCnt = 0;

    if (searchResult.IsDefined()) {
        normCnt += ComputeSimilarity(resultTrackNameSimilarity, searchText, searchResult, "title");
        normCnt += ComputeSimilarity(resultAlbumNameSimilarity, searchText, searchResult, "album.title");
        normCnt +=
            ComputeAggregatedSimilarity(resultArtistNameSimilarity, searchText, searchResult, "artists", "name");
    }

    // NOTE(a-square): if you need RandomLog fields, make sure BASS doesn't strip them from the output!
    const auto& factorsData = searchResult["factorsData"];
    if (!factorsData.IsDefined()) {
        return normCnt;
    }

    const auto& wizardData = factorsData["wizards"]["musicplayer"];
    if (wizardData.IsDefined()) {
        if (const auto& document = wizardData["document"]; document.IsDefined()) {
            normCnt += ComputeSimilarity(wizardTitleSimilarity, searchText, document, "doctitle");
        }

        normCnt += ComputeSimilarity(wizardTrackNameSimilarity, searchText, wizardData, "track_name");
        normCnt += ComputeSimilarity(wizardTrackLyricsSimilarity, searchText, wizardData, "track_lyrics");
        normCnt += ComputeSimilarity(wizardAlbumNameSimilarity, searchText, wizardData, "alb_name");
        normCnt += ComputeAggregatedSimilarity(wizardArtistNameSimilarity, searchText, wizardData, "grp", "name");

        if (const auto* pos = GetMapValue(wizardData, "pos")) {
            features.MutableWizard()->SetPosDCG(CalcDCG(pos->GetInteger() + 1));
        }
    }

    normCnt += ComputeAggregatedSimilarity(documentsTitleSimilarity, searchText, factorsData, "documents", "doctitle");
    normCnt += ComputeAggregatedSimilarity(documentsSnippetSimilarity, searchText, factorsData, "documents",
                                           "passages");
    return normCnt;
}

} // namespace NAlice::NHollywood
