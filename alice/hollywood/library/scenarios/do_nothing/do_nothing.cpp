#include "do_nothing.h"

#include <alice/hollywood/library/scenarios/do_nothing/nlg/register.h>

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

void TDoNothingRunHandle::Do(TScenarioHandleContext& ctx) const {
    TRunResponseBuilder builder;
    builder.CreateResponseBodyBuilder();
    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO(
    "do_nothing",
    AddHandle<TDoNothingRunHandle>()
        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NDoNothing::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood
