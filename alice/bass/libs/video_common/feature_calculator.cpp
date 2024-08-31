#include "feature_calculator.h"

#include "item_selector.h"

#include <alice/library/response_similarity/response_similarity.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NBASS::NVideoCommon {

using namespace NAlice;
using namespace NAlice::NResponseSimilarity;
using namespace NAlice::NVideoCommon;

namespace {

THashSet<TStringBuf> SEARCH_INTENTS = {SEARCH_VIDEO, SEARCH_VIDEO_ENTITY};

const THashMap<TStringBuf, std::function<void(TVideoFeatures&)>> ATTENTION_SETTERS{
    {ATTENTION_EMPTY_SEARCH_GALLERY, [](TVideoFeatures& features) { features.SetIsGalleryEmpty(true); }},
    {ATTENTION_NO_GOOD_RESULT, [](TVideoFeatures& features) { features.SetHasGoodResult(false); }},
    {ATTENTION_ALL_RESULTS_FILTERED, [](TVideoFeatures& features) { features.SetAreAllResultsFiltered(true); }},
    {ATTENTION_AUTOPLAY, [](TVideoFeatures& features) { features.SetAutoplay(true); }},
    {ATTENTION_AUTOSELECT, [](TVideoFeatures& features) { features.SetAutoselect(true); }},
    {ATTENTION_NO_SUCH_SEASON, [](TVideoFeatures& features) { features.SetNoSuchSeason(true); }},
    {ATTENTION_NO_SUCH_EPISODE, [](TVideoFeatures& features) { features.SetNoSuchEpisode(true); }},
    {ATTENTION_NON_AUTHORIZED_USER, [](TVideoFeatures& features) { features.SetNonAuthorizedUser(true); }}
};

TMaybe<TSimilarity> GetItemNameSimilarity(const NParsedUserPhrase::TParsedSequence& query,
                                          const NSc::TValue& item, const ELanguage lang) {
    if (item.Has("normalized_name")) {
        return CalculateNormalizedResponseItemSimilarity(query, item["normalized_name"].GetString());
    }
    if (item.Has("name")) {
        return CalculateResponseItemSimilarity(query, item["name"].GetString(), lang);
    }
    return Nothing();
}

TMaybe<TSimilarity> GetItemDescriptionSimilarity(const NParsedUserPhrase::TParsedSequence& query,
                                                 const NSc::TValue& item, const ELanguage lang) {
    if (item.Has("description")) {
        return CalculateResponseItemSimilarity(query, item["description"].GetString(), lang);
    }
    return Nothing();
}

void GetItemsFeatures(const TString& searchText, const NSc::TValue& items,
                      TVideoFeatures& features, const ELanguage lang) {
    TVector<TSimilarity> nameSimilarities;
    TVector<TSimilarity> descriptionSimilarities;
    NParsedUserPhrase::TParsedSequence query(searchText);

    for (const auto& item : items.GetArray()) {
        if (const auto nameSimilarity = GetItemNameSimilarity(query, item, lang)) {
            nameSimilarities.emplace_back(std::move(*nameSimilarity));
        }
        if (const auto descriptionSimilarity = GetItemDescriptionSimilarity(query, item, lang)) {
            descriptionSimilarities.emplace_back(std::move(*descriptionSimilarity));
        }
    }

    *features.MutableItemNameSimilarity() = AggregateSimilarity(nameSimilarities);
    *features.MutableItemDescriptionSimilarity() = AggregateSimilarity(descriptionSimilarities);
}

void GetSimilarityFromJson(const NSc::TValue& data, TSimilarity* similarity) {
    const auto decoded = Base64Decode(data.GetString());
    TStringInput input(decoded);
    similarity->ParseFromArcadiaStream(&input);
}

bool TryFindSimilarityFeatures(const NSc::TValue& bassResponse, TVideoFeatures& features) {
    if (!bassResponse.Has("blocks")) {
        return false;
    }

    for (const auto& block : bassResponse["blocks"].GetArray()) {
        if (block["type"].GetString() != VIDEO_FACTORS_BLOCK_TYPE || !block.Has("data")) {
            continue;
        }
        const auto& data = block["data"];
        if (data.Has(VIDEO_FACTORS_NAME_SIMILARITY)) {
            GetSimilarityFromJson(data[VIDEO_FACTORS_NAME_SIMILARITY], features.MutableItemNameSimilarity());
        }
        if (data.Has(VIDEO_FACTORS_DESCRIPTION_SIMILARITY)) {
            GetSimilarityFromJson(data[VIDEO_FACTORS_DESCRIPTION_SIMILARITY], features.MutableItemDescriptionSimilarity());
        }
        return true;
    }
    return false;
}

void CalculateSearchFeatures(TVideoFeatures& features, const TMaybe<TString>& searchText, const NSc::TValue bassResponse,
                             bool isFinished, ELanguage lang, const TLogAdapter& logger) {
    if (!searchText.Defined()) {
        return;
    }
    features.SetIsResponseEntity(bassResponse.Has("video_item"));

    const bool gotSimilarityFactors = TryFindSimilarityFeatures(bassResponse, features);
    if (gotSimilarityFactors) {
        LOG_ADAPTER_DEBUG(logger) << "Got video similarity factors from bass";
        return;
    }

    if (isFinished && bassResponse.Has("blocks")) {
        for (const auto& block : bassResponse["blocks"].GetArray()) {
            if (block["type"].GetString() == "command") {
                if (block.Has("data") && block["data"].Has("items")) {
                    // TODO(tolyandex) https://st.yandex-team.ru/DIALOG-5502
                    GetItemsFeatures(*searchText, block["data"]["items"], features, lang);
                }
            }
        }
    }

    if (!bassResponse.Has("video_item")) {
        return;
    }
    const NParsedUserPhrase::TParsedSequence query(searchText.GetRef());
    const auto& item = bassResponse["video_item"];
    // TODO(tolyandex) https://st.yandex-team.ru/DIALOG-5502
    if (auto nameSimilarity = GetItemNameSimilarity(query, item, lang)) {
        features.MutableItemNameSimilarity()->Swap(nameSimilarity.Get());
    }
    if (auto descriptionSimilarity = GetItemDescriptionSimilarity(query, item, lang)) {
        features.MutableItemDescriptionSimilarity()->Swap(descriptionSimilarity.Get());
    }
}

void CalculateAttentionFeatures(TVideoFeatures& features, const NSc::TValue bassResponse, bool isFinished) {
    if (isFinished && bassResponse.Has("blocks")) {
        for (const auto& block : bassResponse["blocks"].GetArray()) {
            if (block["type"].GetString() == "attention") {
                if (const auto* setter = ATTENTION_SETTERS.FindPtr(block["attention_type"].GetString())) {
                    (*setter)(features);
                }
            }
        }
    }
}

} // namespace

void CalculateFeaturesAtStart(const TStringBuf intentType,
                              const TMaybe<TString>& searchText,
                              TVideoFeatures& features,
                              const TDeviceState& deviceState,
                              float selectorConfidenceByName,
                              float selectorConfidenceByNumber,
                              const TLogAdapter& logger) {
    features.SetIsSearchVideo(intentType == SEARCH_VIDEO);
    features.SetIsSelectVideoFromGallery(intentType == QUASAR_SELECT_VIDEO_FROM_GALLERY);
    features.SetIsPaymentConfirmed(intentType == QUASAR_PAYMENT_CONFIRMED);
    features.SetIsAuthorizeProvider(intentType == QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
    features.SetIsOpenCurrentVideo(intentType == QUASAR_OPEN_CURRENT_VIDEO);
    features.SetIsGoToVideoScreen(intentType == QUASAR_GOTO_VIDEO_SCREEN);

    if (searchText) {
        const TItemSelectionResult selected = SelectVideoFromGallery(deviceState, *searchText, logger);
        features.SetItemSelectorConfidence(selected.Confidence);
        features.SetItemSelectorConfidenceByName(selectorConfidenceByName);
        features.SetItemSelectorConfidenceByNumber(selectorConfidenceByNumber);
    } else {
        features.SetItemSelectorConfidence(1.0);
    }
}

void CalculateFeaturesAtFinish(const TStringBuf intentType,
                               const TMaybe<TString>& searchText,
                               TVideoFeatures& features,
                               const NSc::TValue& bassResponse,
                               bool isFinished,
                               ELanguage lang,
                               const TLogAdapter& logger) {
    if (SEARCH_INTENTS.contains(intentType)) {
        CalculateSearchFeatures(features, searchText, bassResponse, isFinished, lang, logger);
    }
    CalculateAttentionFeatures(features, bassResponse, isFinished);
}

} // namespace NBASS::NVideoCommon
