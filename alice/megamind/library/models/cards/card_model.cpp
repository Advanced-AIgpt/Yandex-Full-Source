#include "card_model.h"

namespace NAlice::NMegamind {

TCardModel::TCardModel(const ECardType type)
    : Type(type) {
}

ECardType TCardModel::GetType() const {
    return Type;
}

} // namespace NAlice::NMegamind
