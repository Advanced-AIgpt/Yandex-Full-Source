#pragma once

#include "client_directive_model.h"

#include <alice/protos/data/external_entity_description.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NMegamind {

class TAddExternalEntitiesDescriptionDirectiveModel final : public TClientDirectiveModel {
public:
    explicit TAddExternalEntitiesDescriptionDirectiveModel();

    void Accept(IModelSerializer& serializer) const override;

    void AddExternalEntityDescription(const NData::TExternalEntityDescription& externalEntityDescription);

    [[nodiscard]] const TVector<NData::TExternalEntityDescription>& GetExternalEntitiesDescription() const {
        return ExternalEntitiesDescription;
    }

private:
    TVector<NData::TExternalEntityDescription> ExternalEntitiesDescription;
};

} // namespace NAlice::NMegamind
