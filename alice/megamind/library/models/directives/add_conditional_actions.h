#pragma once

#include "client_directive_model.h"

#include <alice/megamind/protos/common/conditional_action.pb.h>

#include <util/generic/hash.h>

namespace NAlice::NMegamind {

class TAddConditionalActionsDirectiveModel final : public TClientDirectiveModel {
public:
    explicit TAddConditionalActionsDirectiveModel();

    void Accept(IModelSerializer& serializer) const override;

    void AddConditionalAction(const TString& conditionalActionId, const TConditionalAction& conditionalAction);

    [[nodiscard]] const THashMap<TString, TConditionalAction>& GetConditionalActions() const {
        return ConditionalActions;
    }

private:
    THashMap<TString, TConditionalAction> ConditionalActions;
};

} // namespace NAlice::NMegamind
