#include "reminders.h"

#include <alice/hollywood/library/scenarios/reminders/nlg/register.h>
#include <alice/hollywood/library/scenarios/reminders/scene/old_flow.h>
#include <alice/hollywood/library/scenarios/reminders/scene/vins.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NReminders {

HW_REGISTER(TRemindersScenario);

TRemindersScenario::TRemindersScenario()
    : TScenario("reminders")
{
    Register(&TRemindersScenario::Dispatch);
    RegisterScene<TRemindersOldFlowScene>([this]() {
        RegisterSceneFn(&TRemindersOldFlowScene::Main);
    });
    RegisterScene<TRemindersVinsScene>([this]() {
        RegisterSceneFn(&TRemindersVinsScene::MainSetup);
        RegisterSceneFn(&TRemindersVinsScene::Main);
        RegisterSceneFn(&TRemindersVinsScene::ApplySetup);
        RegisterSceneFn(&TRemindersVinsScene::Apply);
    });

    SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> ScenarioResponse());
    SetApphostGraph(ScenarioApply() >>
                    NodeApplySetup("apply_setup") >>
                    NodeApply("apply_main") >>
                    ScenarioResponse());

    SetProductScenarioName(NProductScenarios::REMINDER);
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NReminders::NNlg::RegisterAll);
}

TRetScene TRemindersScenario::Dispatch(const TRunRequest& runRequest, const TStorage&, const TSource&) const {
    if (TRemindersOldFlowScene::IsRequestSupported(runRequest)) {
        return TReturnValueScene<TRemindersOldFlowScene>(TRemindersOldFlowSceneArgs());
    }
    if (runRequest.Input().GetUserLanguage() == ELanguage::LANG_ARA) {
        return TReturnValueScene<TRemindersVinsScene>(TRemindersVinsSceneArgs());
    }
    return TReturnValueRenderIrrelevant("error", "render_irrelevant");
}

void TRemindersScenario::Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const {
    TRemindersVinsRunRenderArgs remindersRunRenderArgs;
    if (info.RenderArguments && info.RenderArguments->UnpackTo(&remindersRunRenderArgs)) {
        SaveHwfState(runResponse, *remindersRunRenderArgs.MutableScenarioRunResponse());
        runResponse = std::move(*remindersRunRenderArgs.MutableScenarioRunResponse());
    }
}

void TRemindersScenario::Hook(THookInputInfo& info, NScenarios::TScenarioApplyResponse& applyResponse) const {
    TRemindersVinsApplyRenderArgs remindersApplyRenderArgs;
    if (info.RenderArguments && info.RenderArguments->UnpackTo(&remindersApplyRenderArgs)) {
        SaveHwfState(applyResponse, *remindersApplyRenderArgs.MutableScenarioApplyResponse());
        applyResponse = std::move(*remindersApplyRenderArgs.MutableScenarioApplyResponse());
    }
}

}  // namespace NAlice::NHollywood::NReminders
