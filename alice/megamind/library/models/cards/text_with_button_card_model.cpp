#include "text_with_button_card_model.h"

namespace NAlice::NMegamind {

// TTextWithButtonCardModel ----------------------------------------------------
TTextWithButtonCardModel::TTextWithButtonCardModel(const TString& text, TVector<TIntrusivePtr<IButtonModel>> buttons)
    : TCardModel(ECardType::TextWithButton)
    , Text(text)
    , Buttons(std::move(buttons)) {
}

void TTextWithButtonCardModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TTextWithButtonCardModel::GetText() const {
    return Text;
}
const TVector<TIntrusivePtr<IButtonModel>>& TTextWithButtonCardModel::GetButtons() const {
    return Buttons;
}

// TTextWithButtonCardModelBuilder ---------------------------------------------
TTextWithButtonCardModel TTextWithButtonCardModelBuilder::Build() const {
    return TTextWithButtonCardModel(Text, Buttons);
}

TTextWithButtonCardModelBuilder& TTextWithButtonCardModelBuilder::SetText(const TString& text) {
    Text = text;
    return *this;
}

} // namespace NAlice::NMegamind
