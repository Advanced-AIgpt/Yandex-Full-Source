#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/general_conversation/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("general_conversation_tr",
                  AddHandle<TGeneralConversationTrPrepareHandle>()
                  .AddHandle<TGeneralConversationTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NGeneralConversation::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
