#include "item_selector.h"

#include <alice/nlu/libs/item_selector/catboost_item_selector/loader.h>
#include <alice/nlu/libs/item_selector/catboost_item_selector/easy_tagger.h>
#include <alice/nlu/libs/item_selector/relevance_based/lstm_relevance_computer.h>
#include <alice/nlu/libs/item_selector/relevance_based/relevance_based.h>

#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/rnn_tagger/rnn_tagger.h>

namespace NAlice {
namespace {

constexpr size_t TAGGER_TOP_SIZE = 10;

NAlice::NItemSelector::TEasyTagger GetEasyTagger(const TString& taggerPath, const NAlice::TTokenEmbedder& embedder,
                                                 const size_t topSize) {
    return {NVins::TRnnTagger(taggerPath), embedder, topSize};
}

THolder<NAlice::NItemSelector::IItemSelector> LoadVideoItemSelector(
        IInputStream* protobufModel, IInputStream* modelDescription,
        const NAlice::TTokenEmbedder* embedder, IInputStream* specialEmbeddings) {
    return MakeHolder<NAlice::NItemSelector::TRelevanceBasedItemSelector>(
        MakeHolder<NAlice::NItemSelector::TLSTMRelevanceComputer>(
            MakeHolder<NAlice::NItemSelector::TNnComputer>(protobufModel, modelDescription),
            embedder,
            NAlice::NItemSelector::ReadSpecialEmbeddings(specialEmbeddings),
            /* maxTokens = */ 128
        ),
        /* threshold = */ 0.6,
        /* filterItemsByRequestLanguage = */ false
    );
}

} // anonymous namespace

NItemSelector::TCompositeItemSelector LoadSelectors(
        const TString& modelPath, const TString& specPath,
        const TString& taggerPath, const NAlice::TTokenEmbedder& embedder,
        IInputStream* protobufModel, IInputStream* modelDescription, IInputStream* specialEmbeddings,
        THolder<NItemSelector::IItemSelector>&& defaultSelector
) {

    NItemSelector::TSelectorCollection selectorCollection;

    selectorCollection[VIDEO_GALLERY_KEY][CATBOOST_ITEM_SELECTOR] = MakeHolder<NAlice::NItemSelector::TEasyCatboostItemSelector>(
        GetEasyTagger(taggerPath, embedder, /* topSize = */ TAGGER_TOP_SIZE),
        NAlice::NItemSelector::LoadModel(modelPath, specPath)
    );

    selectorCollection[VIDEO_GALLERY_KEY][LSTM_ITEM_SELECTOR] = LoadVideoItemSelector(protobufModel, modelDescription,&embedder, specialEmbeddings);

    return NItemSelector::TCompositeItemSelector(std::move(selectorCollection), std::move(defaultSelector));
}

ELanguage ConvertLanguageEnum(const NAlice::ELang lang) {
    if (lang == NAlice::ELang::L_RUS) {
        return ELanguage::LANG_RUS;
    } else if (lang == NAlice::ELang::L_TUR) {
        return ELanguage::LANG_TUR;
    }
    return ELanguage::LANG_UNK;
}

} // namespace NAlice
