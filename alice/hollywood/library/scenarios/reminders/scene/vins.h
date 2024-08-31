#pragma once

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/reminders/proto/reminders.pb.h>

namespace NAlice::NHollywoodFw::NReminders {

class TRemindersVinsScene : public TScene<TRemindersVinsSceneArgs> {
public:
    TRemindersVinsScene(const TScenario* owner);

    TRetSetup MainSetup(const TRemindersVinsSceneArgs& args,
                        const TRunRequest& request,
                        const TStorage& storage) const override;

    TRetMain Main(const TRemindersVinsSceneArgs& args,
                  const TRunRequest& request,
                  TStorage& storage,
                  const TSource& source) const override;

    TRetSetup ApplySetup(const TRemindersVinsSceneArgs& args,
                         const TApplyRequest& request,
                         const TStorage& storage) const override;

    TRetContinue Apply(const TRemindersVinsSceneArgs& args,
                       const TApplyRequest& request,
                       TStorage& storage,
                       const TSource& source) const override;

    static TRetResponse RenderRun(const TRemindersVinsRunRenderArgs& args,
                                  TRender& render);

    static TRetResponse RenderApply(const TRemindersVinsApplyRenderArgs& args,
                                    TRender& render);
};

}  // namespace NAlice::NHollywoodFw::NReminders
