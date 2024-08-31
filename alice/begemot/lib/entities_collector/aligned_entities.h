#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <util/generic/noncopyable.h>

namespace NBg::NAliceEntityCollector {

// ~~~~ TAlignedEntities ~~~~

class TAlignedEntities : public TMoveOnly {
public:
    explicit TAlignedEntities(const TVector<TString>& tokens);

    // Align and add entities.
    void AddEntities(const TVector<TString>& tokens, TVector<NGranet::TEntity>&& entities);

    const TVector<NGranet::TEntity>& GetEntities() const;

private:
    const TVector<TString> Tokens;
    NNlu::TTokenCachedAligner Aligner;
    TVector<NGranet::TEntity> AlignedEntities;
};

} // namespace NBg::NAliceEntityCollector
