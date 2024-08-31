#include "directive_model.h"

namespace NAlice::NMegamind {

void TBaseDirectiveModel::SetEndpointId(const TString& endpointId) {
    EndpointId = endpointId;
}

const TMaybe<TString>& TBaseDirectiveModel::GetEndpointId() const {
    return EndpointId;
}

} // namespace NAlice::NMegamind
