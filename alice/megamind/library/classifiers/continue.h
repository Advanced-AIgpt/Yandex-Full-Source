#pragma once

#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>

#include <util/generic/strbuf.h>

class TFactorStorage;

namespace NAlice {

class IContext;
class TFormulasStorage;
class TScenarioResponse;

namespace NImpl {

class IFormulaApplier {
public:
    ~IFormulaApplier() = default;
    virtual double Predict(const IContext& ctx, const TFactorStorage& factorStorage,
                           const TFormulasStorage& formulasStorage, TStringBuf scenarioName, TStringBuf experiment,
                           EClientType clientType, const TScenarioResponse& response) const = 0;
};

class TFormulaApplier final : public IFormulaApplier {
public:
    double Predict(const IContext& ctx, const TFactorStorage& factorStorage, const TFormulasStorage& formulasStorage,
                   TStringBuf scenarioName, TStringBuf experiment, EClientType clientType,
                   const TScenarioResponse& response) const override;
};

bool IsScenarioAllowedForEarlyContinue(const IContext& ctx, const TFactorStorage& factorStorage,
                                       const TFormulasStorage& formulasStorage, IFormulaApplier& formulaApplier,
                                       TStringBuf scenarioName, const TScenarioResponse& response);
} // namespace NImpl

bool IsScenarioAllowedForEarlyContinue(const IContext& ctx, const TFactorStorage& factorStorage,
                                       const TFormulasStorage& formulasStorage, TStringBuf scenarioName,
                                       const TScenarioResponse& response);

} // namespace NAlice
