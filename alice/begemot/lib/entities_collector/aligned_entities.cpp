#include "aligned_entities.h"

namespace NBg::NAliceEntityCollector {

TAlignedEntities::TAlignedEntities(const TVector<TString>& tokens)
    : Tokens(tokens)
{
}

// Align entities
void TAlignedEntities::AddEntities(const TVector<TString>& tokens, TVector<NGranet::TEntity>&& entities) {
    if (entities.empty()) {
        return;
    }
    const NNlu::TAlignment alignment = Aligner.Align(tokens, Tokens);
    for (NGranet::TEntity& entity : entities) {
        entity.Interval = alignment.GetMap1To2().ConvertInterval(entity.Interval);
        if (!entity.Interval.Empty()) {
            AlignedEntities.push_back(entity);
        }
    }
}

const TVector<NGranet::TEntity>& TAlignedEntities::GetEntities() const {
    return AlignedEntities;
}

} // namespace NBg::NAliceEntityCollector
