#pragma once

#include "client_directive_model.h"
#include "typed_semantic_frame_request_directive_model.h"

#include <util/generic/hash.h>

namespace NAlice::NMegamind {

class TUpdateSpaceActionsDirectiveModel final : public TClientDirectiveModel {
public:
    using TActionSpaceId = TString;
    using TFrameName = TString;
    using TActionSpaces = THashMap<TActionSpaceId, THashMap<TFrameName, TTypedSemanticFrameRequestDirectiveModel>>;
    using TFrames = THashMap<TFrameName, TTypedSemanticFrameRequestDirectiveModel>;

public:
    explicit TUpdateSpaceActionsDirectiveModel(TActionSpaces&& actionSpaces = {});

    void Accept(IModelSerializer& serializer) const override;

    void AddActionSpace(const TActionSpaceId& actionSpaceId, TFrames&& frames);

    [[nodiscard]] const TActionSpaces& GetActionSpaces() const {
        return ActionSpaces;
    }

private:
    TActionSpaces ActionSpaces;
};

} // namespace NAlice::NMegamind
