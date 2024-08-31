#pragma once

#include <alice/megamind/library/models/interfaces/button_model.h>

namespace NAlice::NMegamind {

class TButtonModel : public virtual IButtonModel {
public:
    TButtonModel(const TString& title, EButtonType type);

    [[nodiscard]] const TString& GetTitle() const final;
    [[nodiscard]] EButtonType GetType() const final;

private:
    const TString Title;
    const EButtonType Type;
};

} // namespace NAlice::NMegamind
