#pragma once

#include <alice/megamind/library/util/guid.h>

namespace NAlice::NMegamind {

class TFakeGuidGenerator : public IGuidGenerator {
public:
    explicit TFakeGuidGenerator(const TString& guid);

    [[nodiscard]] TString GenerateGuid() const final;
    TIntrusivePtr<IGuidGenerator> Clone() const final;

private:
    TString Guid;
};

} // namespace NAlice::NMegamind
