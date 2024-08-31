#include "set_search_filter_directive_model.h"

namespace NAlice::NMegamind {

TSetSearchFilterDirectiveModel::TSetSearchFilterDirectiveModel(const TString& analyticsType, ESearchFilterLevel level)
    : TClientDirectiveModel("set_search_filter", analyticsType)
    , Level(level) {
}

void TSetSearchFilterDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

ESearchFilterLevel TSetSearchFilterDirectiveModel::GetLevel() const {
    return Level;
}

} // namespace NAlice::NMegamind
