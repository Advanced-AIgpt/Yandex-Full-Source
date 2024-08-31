#pragma once

#include <alice/nlu/libs/anaphora_resolver/common/match.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NAlice {
    template <class TMatcher>
    class TAnaphoraFinder {
    public:
        explicit TAnaphoraFinder(TMatcher&& matcher)
            : Matcher(std::move(matcher))
        {
        }

        TVector<TAnaphoraMatch> FindAnaphoraMatches(const TDialogueWithMentions& dialogueWithMentions) const {
            Y_ENSURE(!dialogueWithMentions.DialoguePhrases.empty(), "No request in conversation.");

            TVector<TAnaphoraMatch> matches;
            for (size_t pronounIdx = 0; pronounIdx < dialogueWithMentions.Pronouns.size(); ++pronounIdx) {
                const auto& pronoun = dialogueWithMentions.Pronouns[pronounIdx];
                const auto& grammemes = dialogueWithMentions.PronounGrammemes[pronounIdx];
                const auto match = Matcher.Predict(dialogueWithMentions.DialoguePhrases,
                                                   dialogueWithMentions.Entities,
                                                   pronoun,
                                                   grammemes);
                if (!match.Empty()) {
                    matches.push_back(*match);
                }
            }

            Sort(matches.rbegin(), matches.rend());
            return matches;
        }

    private:
        TMatcher Matcher;
    };
} // namespace NAlice
