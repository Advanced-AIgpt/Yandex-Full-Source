#include "set_cookies_directive_model.h"

namespace NAlice::NMegamind {

TSetCookiesDirectiveModel::TSetCookiesDirectiveModel(const TString& analyticsType, const TString& value)
    : TClientDirectiveModel("set_cookies", analyticsType)
    , Value(value)
{
}

void TSetCookiesDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TSetCookiesDirectiveModel::GetValue() const {
    return Value;
}

} // namespace NAlice::NMegamind
