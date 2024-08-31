#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

namespace NAlice::NMegamind {

enum class EPlayerRewindType {
    Forward /* "forward" */,
    Backward /* "backward" */,
    Absolute /* "absolute" */,
};

class TPlayerRewindDirectiveModel final : public TClientDirectiveModel {
public:
    TPlayerRewindDirectiveModel(const TString& analyticsType, ui64 amount, EPlayerRewindType rewindType);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] ui64 GetAmount() const;
    [[nodiscard]] EPlayerRewindType GetRewindType() const;

private:
    const ui64 Amount;
    const EPlayerRewindType RewindType;
};

} // namespace NAlice::NMegamind
