#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>

namespace NAlice::NMegamind {

class TSetCookiesDirectiveModel final : public TClientDirectiveModel {
public:
    TSetCookiesDirectiveModel(const TString& analyticsType, const TString& value);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetValue() const;

private:
    const TString Value;
};

} // namespace NAlice::NMegamind
