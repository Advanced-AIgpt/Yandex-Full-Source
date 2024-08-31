#include "wizard.h"

#include <alice/nlu/libs/request_normalizer/request_tokenizer.h>

#include <util/generic/vector.h>
#include <util/string/join.h>

namespace NAlice::NVins::NWizard {

void DoClipNormalization(TString& text) {
    TVector<TStringBuf> tokens = NNlu::TRequestTokenizer::Tokenize(text);

    if (tokens.size() > CLIP_NORMALIZE_SIZE) {
        tokens.erase(tokens.begin() + CLIP_NORMALIZE_SIZE, tokens.end());
    }

    text = JoinSeq(" ", tokens);
}

} // namespace NAlice::NVins::NWizard
