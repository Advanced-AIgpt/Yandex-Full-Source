#pragma once

#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <util/system/types.h>

namespace NAlice {
    using TPhraseId = ui64;
    using TTokenPosition = ui64;

    struct TMentionInPhrase {
        enum class EPhraseType {
            Pronoun,
            NounPhrase,
            Other
        };

        NNlu::TInterval TokenSegment;
        EPhraseType PhraseType = EPhraseType::Other;

        TMentionInPhrase() = default;
        explicit TMentionInPhrase(const size_t startToken,
                                  const size_t endToken,
                                  const EPhraseType phraseType = EPhraseType::Other);
    };

    struct TMentionInDialogue {
        TPhraseId PhrasePos = 0;
        TMentionInPhrase MentionInPhrase;

        TMentionInDialogue() = default;
        explicit TMentionInDialogue(const size_t phrasePos,
                                    const size_t startToken,
                                    const size_t endToken,
                                    const TMentionInPhrase::EPhraseType phraseType = TMentionInPhrase::EPhraseType::Other);
    };

    bool operator==(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs);
    bool operator!=(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs);
    bool operator<(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs);

    bool operator==(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs);
    bool operator!=(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs);
    bool operator<(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs);

    struct TDialogueWithMentions {
        TVector<NVins::TSample> DialoguePhrases;
        TVector<NAlice::TMentionInDialogue> Entities;
        TVector<NAlice::TMentionInDialogue> Pronouns;
        TVector<TString> PronounGrammemes;
        size_t OriginalSessionShift = 0;
    };
} // namespace NAlice
