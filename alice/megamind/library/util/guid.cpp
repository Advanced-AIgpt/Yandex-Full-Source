#include "guid.h"

#include <util/generic/guid.h>

namespace NAlice::NMegamind {

TString TGuidGenerator::GenerateGuid() const {
    return CreateGuidAsString();
}

TIntrusivePtr<IGuidGenerator> TGuidGenerator::Clone() const {
    return MakeIntrusive<TGuidGenerator>(*this);
}

} // namespace NAlice::NMegamind
