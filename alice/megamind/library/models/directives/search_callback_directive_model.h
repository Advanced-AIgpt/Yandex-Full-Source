#pragma once

#include "typed_semantic_frame_request_directive_model.h"

namespace NAlice::NMegamind {

class TSearchCallbackDirectiveModel final : public TTypedSemanticFrameRequestDirectiveModel {
public:
    TSearchCallbackDirectiveModel(const TString& query);
};

} // namespace NAlice::NMegamind
