#include "fake_guid_generator.h"

namespace NAlice::NMegamind {

TFakeGuidGenerator::TFakeGuidGenerator(const TString& guid)
    : Guid(guid) {
}

TString TFakeGuidGenerator::GenerateGuid() const {
    return Guid;
}

TIntrusivePtr<IGuidGenerator> TFakeGuidGenerator::Clone() const {
    return MakeIntrusive<TFakeGuidGenerator>(*this);
}

} // namespace NAlice::NMegamind
