#pragma once

#include "client_directive_model.h"

#include <alice/megamind/library/models/buttons/action_button_model.h>

namespace NAlice::NMegamind {

class TShowButtonsDirectiveModel final : public TClientDirectiveModel {
public:
    TShowButtonsDirectiveModel(const TString& analyticsType,
            TVector<TIntrusivePtr<IButtonModel>> buttons,
            TMaybe<TString> screenId)
        : TClientDirectiveModel("show_buttons", analyticsType)
        , Buttons(std::move(buttons))
        , ScreenId(std::move(screenId))
    {}

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const TVector<TIntrusivePtr<IButtonModel>>& GetButtons() const {
        return Buttons;
    }

    [[nodiscard]] const TMaybe<TString>& GetScreenId() const {
        return ScreenId;
    }

private:
    const TVector<TIntrusivePtr<IButtonModel>> Buttons;
    const TMaybe<TString> ScreenId;
};

} // namespace NAlice::NMegamind
