#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TAutomotiveDemo2019Handler : public IHandler {
public:
    TAutomotiveDemo2019Handler(
        const TStringBuf& experiment,
        const TMap<TStringBuf, TString>& commands,
        const TVector<TStringBuf>& carIds
    )
        : Experiment(experiment)
        , Commands(commands)
        , CarIds(carIds) {}

    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);

private:
    TStringBuf Experiment;
    TMap<TStringBuf, TString> Commands;
    TVector<TStringBuf> CarIds;
};

}
