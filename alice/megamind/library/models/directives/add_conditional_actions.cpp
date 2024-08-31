#include "add_conditional_actions.h"

namespace NAlice::NMegamind {

TAddConditionalActionsDirectiveModel::TAddConditionalActionsDirectiveModel()
    : TClientDirectiveModel(/* name= */ "add_conditional_actions",
                            /* analyticsType= */ "add_conditional_actions") {}

void TAddConditionalActionsDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

void TAddConditionalActionsDirectiveModel::AddConditionalAction(const TString& conditionalActionId,
                                                                const TConditionalAction& conditionalAction) {
    ConditionalActions[conditionalActionId].CopyFrom(conditionalAction);
}

} // namespace NAlice::NMegamind
