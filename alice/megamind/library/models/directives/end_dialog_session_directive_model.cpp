#include "end_dialog_session_directive_model.h"

namespace NAlice::NMegamind {

TEndDialogSessionDirectiveModel::TEndDialogSessionDirectiveModel(const TString& analyticsType, const TString& dialogId)
    : TClientDirectiveModel("end_dialog_session", analyticsType)
    , DialogId(dialogId) {
}

void TEndDialogSessionDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TEndDialogSessionDirectiveModel::GetDialogId() const {
    return DialogId;
}

} // namespace NAlice::NMegamind
