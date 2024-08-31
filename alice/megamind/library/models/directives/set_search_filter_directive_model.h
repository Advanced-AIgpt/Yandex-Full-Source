#pragma once

#include "client_directive_model.h"

namespace NAlice::NMegamind {

enum class ESearchFilterLevel {
    None /* "none" */,
    Strict /* "strict" */,
    Moderate /* "moderate" */,
};

class TSetSearchFilterDirectiveModel final : public TClientDirectiveModel {
public:
    TSetSearchFilterDirectiveModel(const TString& analyticsType, ESearchFilterLevel level);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] ESearchFilterLevel GetLevel() const;

private:
    const ESearchFilterLevel Level;
};

} // namespace NAlice::NMegamind
