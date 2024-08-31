#pragma once

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOnboarding {

    class TGreetingsScene: public TScene<TGreetingsSceneArgs> {
    public:
        TGreetingsScene(const TScenario* owner);

        TRetSetup MainSetup(const TGreetingsSceneArgs&, const TRunRequest&, const TStorage&) const override;

        TRetMain Main(const TGreetingsSceneArgs&,
                      const TRunRequest&,
                      TStorage&,
                      const TSource&) const override;

        static TRetResponse Render(const TGreetingsRenderProto&, TRender&);
    };

}
