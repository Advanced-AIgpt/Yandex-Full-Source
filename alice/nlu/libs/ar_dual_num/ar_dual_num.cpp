#include "ar_dual_num.h"

#include <alice/nlu/libs/normalization/normalize.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NAlice::NNlu {
    
    namespace {
        constexpr TStringBuf NOMINATIVE_SUFFIX = "ان";
        constexpr TStringBuf GENITIVE_SUFFIX = "ين";
    }

    bool IsArDualNum(TStringBuf token) {
        const TString normalizedToken = ::NNlu::NormalizeWord(token, LANG_ARA);
        return normalizedToken.EndsWith(NOMINATIVE_SUFFIX) || normalizedToken.EndsWith(GENITIVE_SUFFIX);
    }

    TFstResult GetArDualNumEntities(const TVector<TString>& tokens) {
        TFstResult result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            TFstEntity& entityProto = *result.AddEntities();
            entityProto.SetStart(i);
            entityProto.SetEnd(i + 1);
            *entityProto.MutableStringValue() = tokens[i];

            if (IsArDualNum(tokens[i])) {
                *entityProto.MutableType() = "NUM";
                *entityProto.MutableValue() = "2";
            } else {
                *entityProto.MutableType() = "";
                *entityProto.MutableValue() = '"' + tokens[i] + '"';
            }
        }
        return result;
    }

} // namespace NAlice::NNlu
