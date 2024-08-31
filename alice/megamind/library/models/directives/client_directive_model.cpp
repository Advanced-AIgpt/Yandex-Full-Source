#include "client_directive_model.h"

namespace NAlice::NMegamind {

TClientDirectiveModel::TClientDirectiveModel(const TString& name, const TString& analyticsType)
    : Name(name)
    , AnalyticsType(analyticsType) {
}

const TString& TClientDirectiveModel::GetName() const {
    return Name;
}

const TString& TClientDirectiveModel::GetAnalyticsType() const {
    return AnalyticsType;
}

EDirectiveType TClientDirectiveModel::GetType() const {
    return EDirectiveType::ClientAction;
}

} // namespace NAlice::NMegamind
