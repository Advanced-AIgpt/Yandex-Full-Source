#pragma once

#include <alice/nlu/libs/item_selector/composite/composite.h>
#include <alice/nlu/libs/item_selector/interface/item_selector.h>
#include <alice/nlu/libs/embedder/embedder.h>

#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>


namespace NAlice {

constexpr TStringBuf CATBOOST_ITEM_SELECTOR = "catboost";
constexpr TStringBuf LSTM_ITEM_SELECTOR = "lstm";

constexpr TStringBuf VIDEO_GALLERY_KEY = "video_gallery";

NItemSelector::TCompositeItemSelector LoadSelectors(
    const TString& modelPath, const TString& specPath,
    const TString& taggerPath, const NAlice::TTokenEmbedder& embedder,
    IInputStream* protobufModel, IInputStream* modelDescription, IInputStream* specialEmbeddings,
    THolder<NItemSelector::IItemSelector>&& defaultSelector
);

ELanguage ConvertLanguageEnum(const NAlice::ELang lang);

} // namespace NAlice
