#include "close_dialog_directive_model.h"

namespace NAlice::NMegamind {

TCloseDialogDirectiveModel::TCloseDialogDirectiveModel(const TString& analyticsType, const TString& dialogId,
                                                       TMaybe<TString>&& screenId)
    : TClientDirectiveModel("close_dialog", analyticsType)
    , DialogId(dialogId)
    , ScreenId(std::move(screenId)) {
}

void TCloseDialogDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TCloseDialogDirectiveModel::GetDialogId() const {
    return DialogId;
}

const TMaybe<TString>& TCloseDialogDirectiveModel::GetScreenId() const {
    return ScreenId;
}

} // namespace NAlice::NMegamind
