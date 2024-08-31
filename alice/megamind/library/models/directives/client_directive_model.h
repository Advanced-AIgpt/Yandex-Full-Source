#pragma once

#include <alice/megamind/library/models/interfaces/directive_model.h>

namespace NAlice::NMegamind {

class TClientDirectiveModel : public TBaseDirectiveModel {
public:
    TClientDirectiveModel(const TString& name, const TString& analyticsType);

    [[nodiscard]] const TString& GetName() const final;
    [[nodiscard]] EDirectiveType GetType() const final;

    [[nodiscard]] const TString& GetAnalyticsType() const;

private:
    TString Name;
    TString AnalyticsType;
};

} // namespace NAlice::NMegamind
