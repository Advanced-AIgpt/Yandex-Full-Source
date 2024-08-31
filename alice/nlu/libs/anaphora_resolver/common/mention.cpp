#include "mention.h"

namespace NAlice {
    TMentionInPhrase::TMentionInPhrase(const size_t startToken, const size_t endToken, const EPhraseType phraseType)
        : TokenSegment{startToken, endToken}
        , PhraseType(phraseType)
    {
    }

    TMentionInDialogue::TMentionInDialogue(const size_t phrasePos,
                                           const size_t startToken,
                                           const size_t endToken,
                                           const TMentionInPhrase::EPhraseType phraseType)
        : PhrasePos(phrasePos)
        , MentionInPhrase(startToken, endToken, phraseType)
    {
    }

    bool operator==(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs) {
        return lhs.TokenSegment == rhs.TokenSegment && lhs.PhraseType == rhs.PhraseType;
    }

    bool operator!=(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs) {
        return !(lhs == rhs);
    }

    bool operator<(const TMentionInPhrase& lhs, const TMentionInPhrase& rhs) {
        return std::tie(lhs.TokenSegment, lhs.PhraseType) < std::tie(rhs.TokenSegment, rhs.PhraseType);
    }

    bool operator==(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs) {
        return lhs.PhrasePos == rhs.PhrasePos && lhs.MentionInPhrase == rhs.MentionInPhrase;
    }

    bool operator!=(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs) {
        return !(lhs == rhs);
    }

    bool operator<(const TMentionInDialogue& lhs, const TMentionInDialogue& rhs) {
        return std::tie(lhs.PhrasePos, lhs.MentionInPhrase) < std::tie(rhs.PhrasePos, rhs.MentionInPhrase);
    }
} // namespace NAlice
