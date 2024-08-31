#pragma once

#include "mention.h"

namespace NAlice {
    struct TAnaphoraMatch {
        TMentionInDialogue Anaphora;
        TMentionInDialogue Antecedent;
        double Score;
        TString AnaphoraGrammemes;
    };

    bool operator<(const TAnaphoraMatch& lhs, const TAnaphoraMatch& rhs);
} // namespace NAlice
