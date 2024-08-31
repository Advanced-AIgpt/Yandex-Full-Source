#pragma once

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/vins/proto/vins.pb.h>

namespace NAlice::NHollywoodFw::NVins {

class TVinsScene : public TScene<TVinsSceneArgs> {
public:
    TVinsScene(const TScenario* owner);

    TRetSetup MainSetup(const TVinsSceneArgs& args,
                        const TRunRequest& request,
                        const TStorage& storage) const override;

    TRetMain Main(const TVinsSceneArgs& args,
                  const TRunRequest& request,
                  TStorage& storage,
                  const TSource& source) const override;

    TRetSetup ApplySetup(const TVinsSceneArgs& args,
                         const TApplyRequest& request,
                         const TStorage& storage) const override;

    TRetContinue Apply(const TVinsSceneArgs& args,
                       const TApplyRequest& request,
                       TStorage& storage,
                       const TSource& source) const override;

    TRetSetup CommitSetup(const TVinsSceneArgs& args,
                          const TCommitRequest& request,
                          const TStorage& storage) const override;

    TRetCommit Commit(const TVinsSceneArgs& args,
                      const TCommitRequest& request,
                      TStorage& storage,
                      const TSource& source) const override;

    static TRetResponse RenderRun(const NScenarios::TScenarioRunResponse& args,
                                  TRender& render);

    static TRetResponse RenderScreenDevice(const TScreenDeviceRender& args,
                                           TRender& render);

    static TRetResponse RenderApply(const NScenarios::TScenarioApplyResponse& args,
                                    TRender& render);
};

}  // namespace NAlice::NHollywoodFw::NVins
