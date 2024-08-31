#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/reminders/proto/reminders.pb.h>

namespace NAlice::NHollywoodFw::NReminders {

class TRemindersOldFlowScene : public TScene<TRemindersOldFlowSceneArgs> {
public:
    explicit TRemindersOldFlowScene(const TScenario* owner)
        : TScene(owner, SceneName)
    {
    }

    TRetMain Main(const TRemindersOldFlowSceneArgs&, const TRunRequest&, TStorage&, const TSource&) const override {
        return TReturnValueDo();
    }

public:
    static bool IsRequestSupported(const TRunRequest& runRequest);
public:
    static inline constexpr TStringBuf SceneName = "old_flow";
};

}  // namespace NAlice::NHollywood::NReminders
