#include "update_space_actions.h"

#include <utility>

namespace NAlice::NMegamind {

TUpdateSpaceActionsDirectiveModel::TUpdateSpaceActionsDirectiveModel(
    TUpdateSpaceActionsDirectiveModel::TActionSpaces&& actionSpaces)
    : TClientDirectiveModel(/* name= */ "update_space_actions", /* analyticsType= */ "update_space_actions")
    , ActionSpaces(std::move(actionSpaces)) {
}

void TUpdateSpaceActionsDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

void TUpdateSpaceActionsDirectiveModel::AddActionSpace(
    const TUpdateSpaceActionsDirectiveModel::TActionSpaceId& actionSpaceId, TFrames&& frames) {
    ActionSpaces[actionSpaceId] = std::move(frames);
}

} // namespace NAlice::NMegamind
