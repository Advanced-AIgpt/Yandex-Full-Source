#pragma once

#include "data_loader.h"
#include "decoder.h"

namespace NAlice {

    class TFstNormalizer {
    public:
        explicit TFstNormalizer(const IDataLoader& denormalizerLoader, const IDataLoader& normalizerLoader);

        TString Normalize(const TString& text) const;

    private:
        static const TVector<TString> BlackList;
        TFstDecoder Denormalizer;
        TFstDecoder Normalizer;
    };

} // namespace NAlice
