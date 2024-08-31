#include "defer_apply_directive_model.h"

namespace NAlice::NMegamind {

TDeferApplyDirectiveModel::TDeferApplyDirectiveModel(const TString& session)
    : TUniproxyDirectiveModel("defer_apply")
    , Session(session) {
}

void TDeferApplyDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TDeferApplyDirectiveModel::GetSession() const {
    return Session;
}

} // namespace NAlice::NMegamind
