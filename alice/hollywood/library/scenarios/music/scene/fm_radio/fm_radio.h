#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/music/fm_radio_resources.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

enum struct EStationStatus {
    Undefined = 0,
    OK = 1,
    Unrecognized = 2,
    Inactive = 3,
    Unavailable = 4,
};

class TMusicScenarioSceneFmRadio : public TScene<TMusicScenarioSceneArgsFmRadio> {
public:
    TMusicScenarioSceneFmRadio(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsFmRadio&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetSetup ContinueSetup(
        const TMusicScenarioSceneArgsFmRadio&,
        const TContinueRequest&,
        const TStorage&) const override;

    TRetContinue Continue(
        const TMusicScenarioSceneArgsFmRadio&,
        const TContinueRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
