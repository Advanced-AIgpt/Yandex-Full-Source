#include "onboarding.h"

#include "greetings_scene.h"
#include "what_can_you_do_scene.h"

#include <alice/hollywood/library/scenarios/onboarding/nlg/register.h>

#include <alice/hollywood/library/request/experiments.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/device_state/device_state.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/layout.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <google/protobuf/descriptor.pb.h>

namespace NAlice::NHollywoodFw::NOnboarding {
    namespace {
        const TString NLG_COMMON_NAME = "onboarding_common";
    }

    HW_REGISTER(TOnboardingScenario);

    TOnboardingScenario::TOnboardingScenario()
        : TScenario(NProductScenarios::ONBOARDING)
    {
        Register(&TOnboardingScenario::Dispatch);
        RegisterScene<TGreetingsScene>([this]() {
            RegisterSceneFn(&TGreetingsScene::MainSetup);
            RegisterSceneFn(&TGreetingsScene::Main);
        });

        RegisterScene<TWhatCanYouDoScene>([this]() {
            RegisterSceneFn(&TWhatCanYouDoScene::MainSetup);
            RegisterSceneFn(&TWhatCanYouDoScene::Main);
        });

        // Additional functions
        SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NOnboarding::NNlg::RegisterAll);

        // Установка графа
        SetApphostGraph(ScenarioRequest() >> NodeRun() >> NodeMain() >> NodeRender() >> ScenarioResponse());
    }

    TRetScene TOnboardingScenario::Dispatch(
        const TRunRequest& runRequest,
        const TStorage&,
        const TSource&) const
    {
        TOnboardingGetGreetingsSemanticFrame greetingsSF;
        const bool greetingsEnabled = runRequest.Input().FindTSF(greetingsSF) &&
            runRequest.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ONBOARDING_ENABLE_GREETINGS);

        TOnboardingWhatCanYouDoSemanticFrame whatCanYouDoSF;
        const bool whatCanYouDoEnabled = runRequest.Input().FindTSF(whatCanYouDoSF) &&
            !runRequest.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_ONBOARDING_DISABLE_WHAT_CAN_YOU_DO);

        if (!greetingsEnabled && !whatCanYouDoEnabled) {
            LOG_ERR(runRequest.Debug().Logger()) << "Semantic frames not found";
            return TReturnValueRenderIrrelevant(ToString(NLG_COMMON_NAME), "nothing_to_do");
        }

        if (whatCanYouDoEnabled) {
            if (runRequest.Client().GetClientInfo().IsSmartSpeaker()) {
                TWhatCanYouDoSceneArgs args;
                args.SetPhraseIndex(whatCanYouDoSF.GetPhraseIndex().GetUInt32Value());
                return TReturnValueScene<TWhatCanYouDoScene>(args, whatCanYouDoSF.GetDescriptor()->options().GetExtension(SemanticFrameName));
            }
            LOG_INFO(runRequest.Debug().Logger()) << "Device does not support what_can_you_do scenario";
        }

        if (greetingsEnabled) {
            if (runRequest.Client().GetInterfaces().GetSupportsCloudUiFilling()) {
                TGreetingsSceneArgs args;
                return TReturnValueScene<TGreetingsScene>(args, greetingsSF.GetDescriptor()->options().GetExtension(SemanticFrameName));
            }
            LOG_INFO(runRequest.Debug().Logger()) << "Device does not support cloud_ui_filling";
        }

        return TReturnValueRenderIrrelevant(ToString(NLG_COMMON_NAME), "notsupported");
    }

} // namespace NAlice::NHollywoodFw::NOnboarding
