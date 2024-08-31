#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioSceneCentaur : public TScene<TMusicScenarioSceneArgsCentaur> {
public:
    TMusicScenarioSceneCentaur(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsCentaur&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetSetup ContinueSetup(
        const TMusicScenarioSceneArgsCentaur&,
        const TContinueRequest&,
        const TStorage&) const override;

    TRetContinue Continue(
        const TMusicScenarioSceneArgsCentaur&,
        const TContinueRequest&,
        TStorage&,
        const TSource&) const override;

    TRetResponse Render(
        const NData::TScenarioData&,
        TRender&) const;
};

} // namespace NAlice::NHollywoodFw::NMusic
