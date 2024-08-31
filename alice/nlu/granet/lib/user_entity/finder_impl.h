#pragma once

#include "dictionary.h"
#include "token.h"
#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <library/cpp/langmask/langmask.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NUserEntity {

// ~~~~ TEntityFinder ~~~~

class TEntityFinder : public TNonCopyable {
public:
    TEntityFinder(const TVector<TString>& requestVariants, TStringBuf destAlignTokens,
        const TLangMask& languages = GRANET_LANGUAGES, IOutputStream* log = nullptr);

    void Find(const TEntityDicts& dicts, TVector<TEntity>* entities);
    void Find(const TEntityDict& dict, TVector<TEntity>* entities);

private:
    struct TRequest {
        TVector<TTokenInfo> Tokens;
        NNlu::TAlignment Alignment;
    };

    struct THypothesis {
        // Index in TEntityDict::Items.
        size_t ItemIndex = NPOS;
        // Interval on dest tokens (tokens of destAlignTokens).
        NNlu::TInterval Interval;
        // Quality of hypothesis to select better one from two conflicted hypotheses.
        double Weight = 1.;
    };

private:
    void GenerateHypotheses(const TEntityDict& dict);
    void GenerateWholeHypotheses(size_t itemIndex, const TVector<TTokenInfo>& tokens);
    void GeneratePartialHypotheses(size_t itemIndex, const TVector<TTokenInfo>& tokens, bool fromEnd);
    bool CanPresentInRequest(const TTokenInfo& token) const;
    void GenerateHypotheses(size_t itemIndex, const TTokenRange& itemRange, int itemStrongWordCount);
    void GenerateHypothesis(size_t itemIndex, const TTokenRange& itemRange,
        const TRequest& request, const TTokenRange& requestRange);
    void FilterHypotheses();
    void WriteEntities(const TEntityDict& dict, TVector<TEntity>* entities) const;
    void DumpHypotheses(TStringBuf caption);
    void CleanUp();

private:
    IOutputStream* Log = nullptr;
    TTokenBuilder TokenBuilder;
    TStringBuf DestAlignTokens;
    TVector<TRequest> Requests;
    ui32 RequestTokenIdRangeEnd = 0;
    TVector<TMaybe<THypothesis>> Hypotheses;
};

} // namespace NGranet::NUserEntity
