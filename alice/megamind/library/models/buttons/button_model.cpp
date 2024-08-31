#include "button_model.h"

namespace NAlice::NMegamind {

TButtonModel::TButtonModel(const TString& title, EButtonType type)
    : Title(title)
    , Type(type) {
}

const TString& TButtonModel::GetTitle() const {
    return Title;
}

EButtonType TButtonModel::GetType() const {
    return Type;
}

} // namespace NAlice::NMegamind
