#include "add_external_entities_description.h"

namespace NAlice::NMegamind {

TAddExternalEntitiesDescriptionDirectiveModel::TAddExternalEntitiesDescriptionDirectiveModel()
    : TClientDirectiveModel(/* name= */ "add_external_entities_description",
                            /* analyticsType= */ "add_external_entities_description") {}

void TAddExternalEntitiesDescriptionDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

void TAddExternalEntitiesDescriptionDirectiveModel::AddExternalEntityDescription(const NData::TExternalEntityDescription& externalEntityDescription) {
    ExternalEntitiesDescription.push_back(externalEntityDescription);
}

} // namespace NAlice::NMegamind
