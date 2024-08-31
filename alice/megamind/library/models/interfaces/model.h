#pragma once

#include <alice/megamind/library/models/interfaces/model_serializer.h>

namespace NAlice::NMegamind {

class IModel {
public:
    virtual ~IModel() = default;
    virtual void Accept(IModelSerializer& serializer) const = 0;
};

} // namespace NAlice::NMegamind
