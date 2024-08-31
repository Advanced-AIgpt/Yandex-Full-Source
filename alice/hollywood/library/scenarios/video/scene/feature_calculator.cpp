#include "feature_calculator.h"

#include <alice/library/response_similarity/response_similarity.h>
#include <alice/library/video_common/defs.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <alice/protos/data/video/video.pb.h>


namespace NAlice::NHollywoodFw::NVideo {

TMaybe<TString> GetTitleFromCarouselItemWrapper(const NTv::TCarouselItemWrapper& item) {
    TString name;
    switch (item.GetItemCase()) {
        case NTv::TCarouselItemWrapper::kVideoItem:
            name = item.GetVideoItem().GetTitle();
            break;
        case NTv::TCarouselItemWrapper::kSearchVideoItem:
            name = item.GetSearchVideoItem().GetTitle();
            break;
        case NTv::TCarouselItemWrapper::kCollectionItem:
            name = item.GetCollectionItem().GetTitle();
            break;
        case NTv::TCarouselItemWrapper::kPersonItem:
            name = item.GetPersonItem().GetName();
            break;
        case NTv::TCarouselItemWrapper::ITEM_NOT_SET:
        default:
            return Nothing();
    }
    return name;
}

TMaybe<TString> GetDescriptionFromCarouselItemWrapper(const NTv::TCarouselItemWrapper& item) {
    TString name;
    switch (item.GetItemCase()) {
        case NTv::TCarouselItemWrapper::kVideoItem:
            name = item.GetVideoItem().GetDescription();
            break;
        case NTv::TCarouselItemWrapper::kPersonItem:
            name = item.GetPersonItem().GetDescription();
            break;
        case NTv::TCarouselItemWrapper::kSearchVideoItem:
        case NTv::TCarouselItemWrapper::kCollectionItem:
        case NTv::TCarouselItemWrapper::ITEM_NOT_SET:
        default:
            return Nothing();
    }
    return name;
}

TMaybe<NResponseSimilarity::TSimilarity> GetItemNameSimilarity(const NParsedUserPhrase::TParsedSequence& query,
                                          const NTv::TCarouselItemWrapper& item, const ELanguage lang) {
    const TMaybe<TString> title = GetTitleFromCarouselItemWrapper(item);
    const auto normalizedTitle = NNlu::TRequestNormalizer::Normalize(lang, title->ConstRef());
    if (!normalizedTitle.Empty()) {
        return NResponseSimilarity::CalculateNormalizedResponseItemSimilarity(query, normalizedTitle);
    }
    if (!title->Empty()) {
        return NResponseSimilarity::CalculateResponseItemSimilarity(query, title->ConstRef(), lang);
    }
    return Nothing();
}

TMaybe<NResponseSimilarity::TSimilarity> GetItemDescriptionSimilarity(const NParsedUserPhrase::TParsedSequence& query,
                                                 const NTv::TCarouselItemWrapper& item, const ELanguage lang) {
    TMaybe<TString> description = GetDescriptionFromCarouselItemWrapper(item);
    if (!description.Empty()) {
        return NResponseSimilarity::CalculateResponseItemSimilarity(query, description->ConstRef(), lang);
    }
    return Nothing();
}

TMaybe<NVideoCommon::TVideoFeatures> FillCarouselSimilarity(
    const NTv::TCarousel& carousel,
    const TString& requestText,
    const ELanguage& lang)
{
    if (requestText.Empty()) {
        return Nothing();
    }
    const auto query =  NParsedUserPhrase::TParsedSequence(requestText);
    TVector<NResponseSimilarity::TSimilarity> nameSimilarities;
    TVector<NResponseSimilarity::TSimilarity> descriptionSimilarities;

    for (auto item: carousel.GetItems()) {
        // TODO(tolyandex) https://st.yandex-team.ru/DIALOG-5502
        const auto nameSimilarity = GetItemNameSimilarity(query, item, lang);
        if (nameSimilarity) {
            nameSimilarities.emplace_back(std::move(*nameSimilarity));
        }
        const auto descriptionSimilarity = GetItemDescriptionSimilarity(query, item, lang);
        if (descriptionSimilarity) {
            descriptionSimilarities.emplace_back(std::move(*descriptionSimilarity));
        }
    }

    auto videoFeatures = NVideoCommon::TVideoFeatures();
    if (!nameSimilarities.empty()) {
        *videoFeatures.MutableItemNameSimilarity() = NResponseSimilarity::AggregateSimilarity(nameSimilarities);
    }
    if (!descriptionSimilarities.empty()) {
        *videoFeatures.MutableItemDescriptionSimilarity() = NResponseSimilarity::AggregateSimilarity(descriptionSimilarities);
    }
    return videoFeatures;
}

TMaybe<NVideoCommon::TVideoFeatures> FillCarouselItemSimilarity(
    const NTv::TCarouselItemWrapper& item,
    const TString& requestText,
    const ELanguage& lang)
{
    if (requestText.Empty()) {
        return Nothing();
    }
    const auto query =  NParsedUserPhrase::TParsedSequence(requestText);

    auto itemSimilarity = GetItemNameSimilarity(query, item, lang);
    auto descriptionSimilarity = GetItemDescriptionSimilarity(query, item, lang);
    auto videoFeatures = NVideoCommon::TVideoFeatures();
    if (itemSimilarity) {
        *videoFeatures.MutableItemNameSimilarity() = itemSimilarity.GetRef();
    }
    if (descriptionSimilarity) {
        *videoFeatures.MutableItemDescriptionSimilarity() = itemSimilarity.GetRef();
    }
    return videoFeatures;
}

// TODO(akormushkin): move attention features to renders
// CalculateAttentionFeatures(features, bassResponse, isFinished);

} // namespace NAlice::NHollywoodFw::NVideo
