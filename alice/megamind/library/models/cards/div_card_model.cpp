#include "div_card_model.h"

#include <utility>

namespace NAlice::NMegamind {

TDivCardModel::TDivCardModel(google::protobuf::Struct body, TMaybe<TString> text)
    : TCardModel(ECardType::DivCard)
    , Body(std::move(body))
    , Text(std::move(text)) {
}

void TDivCardModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const google::protobuf::Struct& TDivCardModel::GetBody() const {
    return Body;
}

const TMaybe<TString>& TDivCardModel::GetText() const {
    return Text;
}

} // namespace NAlice::NMegamind
