#include "fst_normalizer.h"

#include <util/string/cast.h>
#include <util/string/split.h>

namespace NAlice {

    const TVector<TString> TFstNormalizer::BlackList {
        "reverse_conversion.profanity",
        "reverse_conversion.make_substitution_group",
        "number.convert_size",
        "units.converter",
        "reverse_conversion.times"
    };

    TFstNormalizer::TFstNormalizer(const IDataLoader& denormalizerLoader, const IDataLoader& normalizerLoader)
        : Denormalizer(denormalizerLoader, BlackList)
        , Normalizer(normalizerLoader, BlackList)
    {
    }

    TString TFstNormalizer::Normalize(const TString& text) const {
        TString result;
        for (const TStringBuf token : StringSplitter(text).SplitBySet(" \t\n\r")) {
            // TODO: пропускать пустые токены
            uint64_t dummy;
            if (!result.Empty()) {
                result += ' ';
            }
            if (TryFromString(token, dummy)) {
                result += Normalizer.Normalize(ToString(token));
            } else {
                result += token;
            }
        }
        return Denormalizer.Normalize(result);
    }

} // namespace NAlice
