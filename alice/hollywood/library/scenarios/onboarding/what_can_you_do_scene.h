#pragma once

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOnboarding {

class TWhatCanYouDoScene : public TScene<TWhatCanYouDoSceneArgs> {
public:
    TWhatCanYouDoScene(const TScenario *owner);

    TRetSetup MainSetup(const TWhatCanYouDoSceneArgs&, const TRunRequest& request, const TStorage&) const override;

    TRetMain Main(const TWhatCanYouDoSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TWhatCanYouDoRenderProto&, TRender&);
};

} // namespace NAlice::NHollywoodFw::NOnboarding
