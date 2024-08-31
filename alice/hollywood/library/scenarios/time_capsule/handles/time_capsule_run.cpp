#include "time_capsule_run.h"

namespace NAlice::NHollywood::NTimeCapsule {

TTimeCapsuleRunHandle::TTimeCapsuleRunHandle()
    : TimeCapsuleSceneCreators_(
        // WARNING: order is important
        // TryCreate functions assume that they will be called in this order
        {
            MakeHolder<TTimeCapsuleInterruptSceneCreator>(),
            MakeHolder<TTimeCapsuleStartSceneCreator>(),
            MakeHolder<TTimeCapsuleSaveSceneCreator>(),
            MakeHolder<TTimeCapsuleSaveApproveSceneCreator>(),
            MakeHolder<TTimeCapsuleStopSceneCreator>(),
            MakeHolder<TTimeCapsuleQuestionSceneCreator>(),
            MakeHolder<TTimeCapsuleApproveRetrySceneCreator>(),
            MakeHolder<TTimeCapsuleTextRetrySceneCreator>(),
            MakeHolder<TTimeCapsuleInformationSceneCreator>(),
            MakeHolder<TTimeCapsuleOpenSceneCreator>(),
            MakeHolder<TTimeCapsuleRerecordSceneCreator>(),
            MakeHolder<TTimeCapsuleHowLongSceneCreator>(),
            MakeHolder<TTimeCapsuleDeleteSceneCreator>()
        }
    ),
    TimeCapsuleIrrelevantSceneCreator_(MakeHolder<TTimeCapsuleIrrelevantSceneCreator>()) {
}

void TTimeCapsuleRunHandle::Do(TScenarioHandleContext& scenarioCtx) const {
    TTimeCapsuleContext ctx{scenarioCtx};
    for (const auto& timeCapsuleSceneCreator : TimeCapsuleSceneCreators_) {
       if (auto scene = timeCapsuleSceneCreator->TryCreate(ctx)) {
           scene->RenderTo(scenarioCtx);
           return;
       }
    }

    // TimeCapsuleIrrelevantSceneCreator_ isn't part of TimeCapsuleSceneCreator_
    auto irrelevantScene = TimeCapsuleIrrelevantSceneCreator_->TryCreate(ctx);
    Y_ENSURE(irrelevantScene);

    irrelevantScene->RenderTo(scenarioCtx);
}

} // namespace NAlice::NHollywood::NTimeCapsule
