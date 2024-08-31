#include "action_button_model.h"

namespace NAlice::NMegamind {

// TActionButtonModel ----------------------------------------------------------
TActionButtonModel::TActionButtonModel(const TString& title, TVector<TIntrusivePtr<IDirectiveModel>> directives,
                                       const TMaybe<TTheme>& theme, const TMaybe<TString>& text)
    : TButtonModel(title, theme.Defined() ? EButtonType::ThemedAction : EButtonType::Action)
    , Directives(std::move(directives))
    , Theme(theme)
    , Text(text) {
}

void TActionButtonModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TVector<TIntrusivePtr<IDirectiveModel>>& TActionButtonModel::GetDirectives() const {
    return Directives;
}

// TActionButtonModelBuilder ---------------------------------------------------
TActionButtonModel TActionButtonModelBuilder::Build() const {
    return TActionButtonModel(Title, Directives);
}

TActionButtonModelBuilder& TActionButtonModelBuilder::SetTitle(const TString& title) {
    Title = title;
    return *this;
}

} // namespace NAlice::NMegamind
