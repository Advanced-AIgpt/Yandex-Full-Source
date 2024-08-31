#include "text_card_model.h"

namespace NAlice::NMegamind {

TTextCardModel::TTextCardModel(const TString& text)
    : TCardModel(ECardType::SimpleText)
    , Text(text) {
}

void TTextCardModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TTextCardModel::GetText() const {
    return Text;
}

} // namespace NAlice::NMegamind
