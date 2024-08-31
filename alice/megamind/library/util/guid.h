#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

class IGuidGenerator : public virtual TThrRefBase {
public:
    ~IGuidGenerator() override = default;
    [[nodiscard]] virtual TString GenerateGuid() const = 0;
    [[nodiscard]] virtual TIntrusivePtr<IGuidGenerator> Clone() const = 0;
};

class TGuidGenerator final : public IGuidGenerator {
public:
    [[nodiscard]] TString GenerateGuid() const final;
    [[nodiscard]] TIntrusivePtr<IGuidGenerator> Clone() const final;
};

} // namespace NAlice::NMegamind
