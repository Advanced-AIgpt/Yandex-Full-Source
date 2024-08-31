#include "frame_filler.h"

#include <alice/nlu/libs/phrase_matching/phrase_matching.h>

#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

namespace NBg {
    namespace {
        const TString NO_REQUESTED_SLOT_NAME = "no_slot";

        TString GetRequestedSlotName(const NAlice::TSemanticFrame& frame) {
            for (const auto& slot : frame.GetSlots()) {
                if (slot.GetIsRequested()) {
                    return slot.GetName();
                }
            }
            return NO_REQUESTED_SLOT_NAME;
        }

        THashSet<TString> GetRequestedSlotAcceptedTypes(const NAlice::TSemanticFrame& frame) {
            for (const auto& slot : frame.GetSlots()) {
                if (!slot.GetIsRequested()) {
                    continue;
                }
                const auto& acceptedTypes = slot.GetAcceptedTypes();
                return THashSet<TString>(acceptedTypes.begin(), acceptedTypes.end());
            }
            return {};
        }
    } // namespace anonymous

    bool TFrameFiller::TSlotMatch::operator<(const TSlotMatch& other) const {
        if (!Score.Defined()) {
            return other.Score.Defined() || NumTokens < other.NumTokens;
        }
        return other.Score.Defined() && *Score < *other.Score;
    }

    TFrameFiller::TFrameFiller(const NAlice::TSemanticFrame& semanticFrame,
                               const TString& slotName,
                               const THashSet<TString>& slotAcceptedTypes,
                               const NNlu::TAlignment& requestToUnresolvedRequestAlignment,
                               const size_t numUnresolvedRequestTokens)
        : SemanticFrame(semanticFrame)
        , SlotName(slotName)
        , SlotAcceptedTypes(slotAcceptedTypes)
        , RequestToUnresolvedRequestAlignment(requestToUnresolvedRequestAlignment)
        , NumUnresolvedRequestTokens(numUnresolvedRequestTokens) {
    }

    TFrameFiller::TFrameFiller(const NAlice::TSemanticFrame& semanticFrame,
                               const NNlu::TAlignment& requestToUnresolvedRequestAlignment,
                               const size_t numUnresolvedRequestTokens)
        : TFrameFiller(semanticFrame,
                       GetRequestedSlotName(semanticFrame),
                       GetRequestedSlotAcceptedTypes(semanticFrame),
                       requestToUnresolvedRequestAlignment,
                       numUnresolvedRequestTokens) {
    }

    size_t TFrameFiller::GetNumTokens(const NNlu::TInterval& resolvedInterval,
                                      const NNlu::TAlignment& alignment) const {
        const auto unresolvedInterval = alignment.GetMap1To2().ConvertInterval(resolvedInterval);
        return unresolvedInterval.Length();
    }

    NAlice::TSemanticFrame TFrameFiller::FillFrame(const TSlotMatch& slotMatch) const {
        NAlice::TSemanticFrame newFrame;
        newFrame.CopyFrom(SemanticFrame);

        for (auto& slot : *newFrame.MutableSlots()) {
            if (slot.GetName() != SlotName) {
                continue;
            }
            slot.SetType(slotMatch.Type);
            slot.SetValue(slotMatch.Value);
            slot.SetIsFilled(true);
            break;
        }

        return newFrame;
    }

    TMaybe<TFrameFiller::TSlotMatch> TFrameFiller::TryFillWithEntity(const TVector<NGranet::TEntity>& entities) const {
        if (SlotAcceptedTypes.empty()) {
            return Nothing();
        }

        TMaybe<TSlotMatch> bestMatch = Nothing();

        for (const auto& entity : entities) {
            const size_t numTokens = GetNumTokens(entity.Interval, RequestToUnresolvedRequestAlignment);
            if (!IsAcceptableTokenSegment(numTokens)) {
                continue;
            }

            if (!SlotAcceptedTypes.contains(entity.Type)) {
                continue;
            }

            UpdateSlotMatchIfBetter({numTokens, entity.Type, entity.Value}, &bestMatch);
        }

        return bestMatch;
    }

    TMaybe<TFrameFiller::TSlotMatch> TFrameFiller::TryFillWithUntypedValue(
        const TVector<NGranet::TEntity>& entities,
        const TVector<TString>& requestTokens
    ) const {
        if (!SlotAcceptedTypes.contains("string")) {
            return Nothing();
        }

        TVector<NNlu::TInterval> nonsense;
        for (const auto& entity : entities) {
            if (entity.Type != "nonsense") {
                continue;
            }
            const NNlu::TInterval unresolvedInterval = RequestToUnresolvedRequestAlignment.GetMap1To2().ConvertInterval(entity.Interval);
            if (unresolvedInterval.Empty()) {
                continue;
            }
            nonsense.push_back(unresolvedInterval);
        }
        nonsense.push_back(NNlu::TInterval{requestTokens.size(), requestTokens.size()});
        Sort(nonsense);

        size_t tokenIndex = 0;
        TVector<TString> meaningfulTokens;
        for (const NNlu::TInterval& nonsenseInterval : nonsense) {
            while (tokenIndex < requestTokens.size() && tokenIndex < nonsenseInterval.Begin) {
                meaningfulTokens.push_back(requestTokens[tokenIndex]);
                ++tokenIndex;
            }
            tokenIndex = nonsenseInterval.End;
        }

        if (meaningfulTokens.empty()) {
            return Nothing();
        }
        return TSlotMatch{meaningfulTokens.size(), "string", JoinSeq(" ", meaningfulTokens)};
    }

    void TFrameFiller::UpdateSlotMatchIfBetter(const TSlotMatch& newMatch, TMaybe<TSlotMatch>* currentBestMatch) {
        Y_ASSERT(currentBestMatch);

        if (!currentBestMatch->Defined() || **currentBestMatch < newMatch) {
            *currentBestMatch = newMatch;
        }
    }

    bool TFrameFiller::IsAcceptableTokenSegment(const size_t unresolvedSegmentLength) const {
        return unresolvedSegmentLength * 2 >= NumUnresolvedRequestTokens;
    }
} // namespace NBg
