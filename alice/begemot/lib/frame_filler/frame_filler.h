#pragma once
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/libs/token_aligner/alignment.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NBg {
    TString GetScoreSlotName(const TString& fillingSlotName);

    class TFrameFiller {
    public:
        struct TSlotMatch {
            size_t NumTokens;
            TString Type;
            TString Value;
            TMaybe<double> Score = Nothing();

            bool operator<(const TSlotMatch& other) const;
        };

    public:
        TFrameFiller() = delete;

        TFrameFiller(const NAlice::TSemanticFrame& semanticFrame,
                     const TString& slotName,
                     const THashSet<TString>& slotAcceptedTypes,
                     const NNlu::TAlignment& requestToUnresolvedRequestAlignment,
                     const size_t numUnresolvedRequestTokens);

        TFrameFiller(const NAlice::TSemanticFrame& semanticFrame,
                     const NNlu::TAlignment& requestToUnresolvedRequestAlignment,
                     const size_t numUnresolvedRequestTokens);

        TMaybe<TSlotMatch> TryFillWithEntity(const TVector<NGranet::TEntity>& entities) const;
        TMaybe<TSlotMatch> TryFillWithUntypedValue(const TVector<NGranet::TEntity>& entities, const TVector<TString>& requestTokens) const;

        NAlice::TSemanticFrame FillFrame(const TSlotMatch& slotMatch) const;
        static void UpdateSlotMatchIfBetter(const TSlotMatch& newMatch, TMaybe<TSlotMatch>* currentBestMatch);

    private:
        size_t GetNumTokens(const NNlu::TInterval& resolvedInterval, const NNlu::TAlignment& alignment) const;
        bool IsAcceptableTokenSegment(const size_t unresolvedSegmentLength) const;

    private:
        const NAlice::TSemanticFrame SemanticFrame;
        const TString SlotName;
        const THashSet<TString> SlotAcceptedTypes;
        const NNlu::TAlignment RequestToUnresolvedRequestAlignment;
        const NNlu::TAlignment MarkupToUnresolvedRequestAlignment;
        const size_t NumUnresolvedRequestTokens = 0;
    };
} // namespace NBg
