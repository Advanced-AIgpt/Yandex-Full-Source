#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_fast_data.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/continue_init_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/continue_candidates_aggregator_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/continue_candidates_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/continue_render_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/prepare_commit_social_sharing_link.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/run_candidates_aggregator_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/run_candidates_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/run_init_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/handles/run_render_handle.h>
#include <alice/hollywood/library/scenarios/general_conversation/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NGeneralConversation {

REGISTER_SCENARIO(ToString(GENERAL_CONVERSATION_SCENARIO_NAME),
                  AddHandle<TGeneralConversationInitHandle>()
                  .AddHandle<TGeneralConversationCandidatesAggregatorHandle>()
                  .AddHandle<TGeneralConversationCandidatesHandle>()
                  .AddHandle<TGeneralConversationRenderHandle>()
                  .AddHandle<TGeneralConversationContinueInitHandle>()
                  .AddHandle<TGeneralConversationContinueCandidatesAggregatorHandle>()
                  .AddHandle<TGeneralConversationContinueCandidatesHandle>()
                  .AddHandle<TGeneralConversationContinueRenderHandle>()
                  .AddHandle<TGeneralConversationPrepareCommitSocialSharingLinkHandle>()
                  .SetResources<TGeneralConversationResources>()
                  .AddFastData<TGeneralConversationFastDataProto, TGeneralConversationFastData>("general_conversation/general_conversation.pb")
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NGeneralConversation::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NGeneralConversation
