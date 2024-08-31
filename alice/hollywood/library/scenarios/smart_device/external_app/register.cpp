#include "common_run_render.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/smart_device/external_app/nlg/register.h>

namespace NAlice::NHollywood {

    REGISTER_SCENARIO(
            "smart_device_external_app",
            AddHandle<TCommonrunRenderHandle>()
            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSmartDevice::NExternalApp::NNlg::RegisterAll)
    );

} // namespace NAlice::NHollywood
