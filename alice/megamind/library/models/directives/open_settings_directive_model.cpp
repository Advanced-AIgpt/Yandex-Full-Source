#include "open_settings_directive_model.h"

namespace NAlice::NMegamind {

TOpenSettingsDirectiveModel::TOpenSettingsDirectiveModel(const TString& analyticsType, ESettingsTarget target)
    : TClientDirectiveModel("open_settings", analyticsType)
    , Target(target) {
}

void TOpenSettingsDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

ESettingsTarget TOpenSettingsDirectiveModel::GetTarget() const {
    return Target;
}

} // namespace NAlice::NMegamind
