#include "occurrence_searcher.h"
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <dict/dictutil/dictutil.h>
#include <util/charset/wide.h>
#include <util/string/strip.h>

namespace NAlice {
namespace NNlu {
    TString NormalizeString(ELanguage language, TStringBuf string) {
        const TString prenormalized = ::NNlu::TRequestNormalizer::Normalize(language, string);
        return Strip(Collapse(WideToUTF8(RemoveDiacritics(language, UTF8ToWide(prenormalized)))));
    }
} // namespace NNlu
} // namespace NAlice
