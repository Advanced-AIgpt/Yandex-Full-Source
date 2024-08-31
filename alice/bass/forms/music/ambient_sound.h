#pragma once

#include <alice/bass/forms/vins.h>

#include <utility>

namespace NBASS {

class TAmbientSoundHandler: public IHandler {
public:
    explicit TAmbientSoundHandler(TString productScenarioName)
        : ProductScenarioName(std::move(productScenarioName)) {
    }

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

protected:
    TResultValue CreateCatalogAnswer(TContext& ctx, TStringBuf objectType, TStringBuf objectValue);

private:
    TString ProductScenarioName;
};

}
