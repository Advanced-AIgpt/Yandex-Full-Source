#pragma once

#include <alice/nlu/libs/anaphora_resolver/common/mention.h>
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <search/begemot/core/rulecontext.h>
#include <search/begemot/rules/alice/anaphora_matcher/proto/alice_anaphora_matcher.pb.h>
#include <search/begemot/rules/alice/session/proto/alice_session.pb.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>

namespace NBg {
    struct TMentionExtractionOptions {
        static constexpr size_t DEFAULT_MAX_SEGMENT_SIZE = 4;
        static constexpr size_t DEFAULT_MAX_HISTORY_LENGTH = 5;

        bool UseGeneratedEntityPositions = true;
        bool UseBegemotMarkupPronounPositions = true;
        size_t MaxGeneratedEntityTokenCount = DEFAULT_MAX_SEGMENT_SIZE;
        size_t MaxHistoryLength = DEFAULT_MAX_HISTORY_LENGTH;
    };

    void AddGrammarAnalysisToPhrase(NProto::TAlicePhrase* phrase);

    template <typename TSessionProto>
    void AddGrammarAnalysis(TSessionProto* sessionProto) {
        Y_ASSERT(sessionProto);

        for (auto& phrase : *sessionProto->MutableDialoguePhrases()) {
            AddGrammarAnalysisToPhrase(&phrase);
        }
        AddGrammarAnalysisToPhrase(sessionProto->MutableNormalizedRequest());
    }

    void GenerateAllSegmentsAsEntities(const NVins::TSample& phrase,
                                       const size_t phraseIdx,
                                       const size_t maxSegmentLength,
                                       TVector<NAlice::TMentionInDialogue>* entities);

    NVins::TSample ConvertProtoPhraseToSample(const NProto::TAlicePhrase& phraseProto);

    void ParsePronounMentions(const NProto::TAlicePhrase& requestProto,
                              const size_t phraseIdx,
                              TVector<NAlice::TMentionInDialogue>* mentions,
                              TVector<TString>* pronounGrammemes);

    template <typename TSessionProto>
    TVector<NVins::TSample> ExtractDialoguePhrases(size_t beginIndex,
                                                   const TSessionProto& sessionProto) {
        TVector<NVins::TSample> dialoguePhrases;
        for (size_t phraseIndex = beginIndex; phraseIndex < sessionProto.DialoguePhrasesSize(); ++phraseIndex) {
            dialoguePhrases.push_back(ConvertProtoPhraseToSample(sessionProto.GetDialoguePhrases(phraseIndex)));
        }
        return dialoguePhrases;
    }

    template <typename TSessionProto>
    NAlice::TDialogueWithMentions ExtractMentions(const TMentionExtractionOptions& options,
                                                  const TSessionProto& sessionProto) {
        NAlice::TDialogueWithMentions result;

        const size_t maxPreviousPhrasesCount = options.MaxHistoryLength - 1;

        result.OriginalSessionShift = 0;
        if (sessionProto.DialoguePhrasesSize() > maxPreviousPhrasesCount) {
            result.OriginalSessionShift = sessionProto.DialoguePhrasesSize() - maxPreviousPhrasesCount;
        }

        result.DialoguePhrases = ExtractDialoguePhrases(result.OriginalSessionShift, sessionProto);
        for (size_t phraseIdx = 0; phraseIdx < result.DialoguePhrases.size(); ++phraseIdx) {
            if (options.UseGeneratedEntityPositions) {
                GenerateAllSegmentsAsEntities(result.DialoguePhrases[phraseIdx],
                                              phraseIdx,
                                              options.MaxGeneratedEntityTokenCount,
                                              &result.Entities);
            }
        }

        const auto& requestProto = sessionProto.GetNormalizedRequest();
        const auto request = ConvertProtoPhraseToSample(requestProto);
        result.DialoguePhrases.push_back(request);
        if (options.UseBegemotMarkupPronounPositions) {
            ParsePronounMentions(requestProto,
                                 /*phraseIdx*/result.DialoguePhrases.size() - 1,
                                 &result.Pronouns,
                                 &result.PronounGrammemes);
        }
        Sort(result.Pronouns);

        return result;
    }

    template <typename TSessionProto>
    NVins::TSample GetNextOriginalPhrase(const TSessionProto& sessionProto, size_t dialogLength,
                                         size_t originalPhrasesLength, size_t phraseIndex) {
        Y_ENSURE(dialogLength >= phraseIndex);

        const size_t dialogPhrasesOffset = dialogLength - phraseIndex;
        if (originalPhrasesLength < dialogPhrasesOffset) {
            return {};
        }

        const size_t dialogPhraseIndex = originalPhrasesLength - dialogPhrasesOffset;
        return ConvertProtoPhraseToSample(sessionProto.GetDialoguePhrases(dialogPhraseIndex));
    }

    template <typename TSessionProto>
    NVins::TSample GetNextRequest(const TSessionProto& sessionProto,
                                  size_t dialogLength, size_t originalPhrasesLength,
                                  size_t rewrittenRequestsLength, size_t phraseIndex) {
        Y_ENSURE(dialogLength >= phraseIndex);

        const size_t rewrittenRequestOffset = (dialogLength - phraseIndex) / 2;
        if (rewrittenRequestsLength >= rewrittenRequestOffset) {
            const size_t rewrittenRequestIndex = rewrittenRequestsLength - rewrittenRequestOffset;

            const auto& protoPhrase = sessionProto.GetPreviousRewrittenRequests(rewrittenRequestIndex);
            if (protoPhrase.TokensSize() != 1 || protoPhrase.GetTokens(0) != "") {
                // otherwise the phrase is empty - fallback to original request
                return ConvertProtoPhraseToSample(protoPhrase);
            }
        }

        return GetNextOriginalPhrase(sessionProto, dialogLength, originalPhrasesLength, phraseIndex);
    }

    template <typename TSessionProto>
    TVector<NVins::TSample> ExtractDialoguePhrasesForRewriter(const TSessionProto& sessionProto) {
        const size_t originalPhrasesLength = sessionProto.DialoguePhrasesSize();
        const size_t rewrittenRequestsLength = sessionProto.PreviousRewrittenRequestsSize();

        const size_t dialogLength = Max(originalPhrasesLength, 2 * rewrittenRequestsLength);

        TVector<NVins::TSample> dialogHistory(Reserve(dialogLength));

        for (size_t phraseIndex = 0; phraseIndex < dialogLength; ++phraseIndex) {
            if (phraseIndex % 2 == 0) {
                dialogHistory.push_back(GetNextRequest(sessionProto, dialogLength, originalPhrasesLength,
                                                       rewrittenRequestsLength, phraseIndex));
            } else {
                dialogHistory.push_back(GetNextOriginalPhrase(sessionProto, dialogLength,
                                                              originalPhrasesLength, phraseIndex));
            }
        }

        dialogHistory.push_back(ConvertProtoPhraseToSample(sessionProto.GetNormalizedRequest()));

        return dialogHistory;
    }
} // namespace NAlice
