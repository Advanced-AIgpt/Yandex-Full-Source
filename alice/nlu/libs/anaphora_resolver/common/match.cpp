#include "match.h"

namespace NAlice {
    bool operator<(const TAnaphoraMatch& lhs, const TAnaphoraMatch& rhs) {
        return std::tie(lhs.Score, lhs.Antecedent, lhs.Anaphora, lhs.AnaphoraGrammemes) <
               std::tie(rhs.Score, rhs.Antecedent, rhs.Anaphora, lhs.AnaphoraGrammemes);
    }
} // namespace NAlice
