#include "uniproxy_directive_model.h"

namespace NAlice::NMegamind {

TUniproxyDirectiveModel::TUniproxyDirectiveModel(const TString& name)
    : Name(name) {
}

const TString& TUniproxyDirectiveModel::GetName() const {
    return Name;
}

EDirectiveType TUniproxyDirectiveModel::GetType() const {
    return EDirectiveType::UniproxyAction;
}

void TUniproxyDirectiveModel::SetUniproxyDirectiveMeta(NSpeechKit::TUniproxyDirectiveMeta uniproxyDirectiveMeta) {
    UniproxyDirectiveMeta = std::move(uniproxyDirectiveMeta);
}

const NSpeechKit::TUniproxyDirectiveMeta* TUniproxyDirectiveModel::GetUniproxyDirectiveMeta() const {
    return UniproxyDirectiveMeta.Get();
}

} // namespace NAlice::NMegamind
