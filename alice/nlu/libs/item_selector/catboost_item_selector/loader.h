#pragma once

#include "catboost_item_selector.h"

namespace NAlice {
namespace NItemSelector {

TCatboostItemSelectorSpec ReadSpec(const TString& path);

TCatboostItemSelector LoadModel(const TString& modelPath, const TString& specPath);

TEasyTagger GetEasyTagger(const TString& taggerPath, const TString& embeddingsPath,
                          const TString& dictionaryPath, const size_t topSize);

} // namespace NItemSelector
} // namespace NAlice
