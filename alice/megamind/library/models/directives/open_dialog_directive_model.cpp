#include "open_dialog_directive_model.h"

namespace NAlice::NMegamind {

// TOpenDialogDirectiveModel ---------------------------------------------------
TOpenDialogDirectiveModel::TOpenDialogDirectiveModel(const TString& analyticsType, const TString& dialogId,
                                                     TVector<TIntrusivePtr<IDirectiveModel>> directives)
    : TClientDirectiveModel("open_dialog", analyticsType)
    , DialogId(dialogId)
    , Directives(std::move(directives)) {
}

void TOpenDialogDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TOpenDialogDirectiveModel::GetDialogId() const {
    return DialogId;
}

const TVector<TIntrusivePtr<IDirectiveModel>>& TOpenDialogDirectiveModel::GetDirectives() const {
    return Directives;
}

// TOpenDialogDirectiveModelBuilder --------------------------------------------
TOpenDialogDirectiveModel TOpenDialogDirectiveModelBuilder::Build() const {
    return TOpenDialogDirectiveModel(AnalyticsType, DialogId, Directives);
}

TOpenDialogDirectiveModelBuilder& TOpenDialogDirectiveModelBuilder::SetAnalyticsType(const TString& analyticsType) {
    AnalyticsType = analyticsType;
    return *this;
}

TOpenDialogDirectiveModelBuilder& TOpenDialogDirectiveModelBuilder::SetDialogId(const TString& dialogId) {
    DialogId = dialogId;
    return *this;
}

} // namespace NAlice::NMegamind
