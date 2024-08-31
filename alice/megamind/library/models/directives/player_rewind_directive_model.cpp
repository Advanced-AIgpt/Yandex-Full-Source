#include "player_rewind_directive_model.h"

namespace NAlice::NMegamind {

TPlayerRewindDirectiveModel::TPlayerRewindDirectiveModel(const TString& analyticsType, ui64 amount,
                                                         EPlayerRewindType rewindType)
    : TClientDirectiveModel("player_rewind", analyticsType)
    , Amount(amount)
    , RewindType(rewindType) {
}

void TPlayerRewindDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

ui64 TPlayerRewindDirectiveModel::GetAmount() const {
    return Amount;
}

EPlayerRewindType TPlayerRewindDirectiveModel::GetRewindType() const {
    return RewindType;
}

} // namespace NAlice::NMegamind
