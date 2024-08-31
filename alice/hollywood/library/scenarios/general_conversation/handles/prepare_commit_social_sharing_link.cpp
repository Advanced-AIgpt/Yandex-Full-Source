#include "prepare_commit_social_sharing_link.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/paskills/social_sharing/proto/api/api.pb.h>

using namespace NAlice::NSocialSharing;

namespace NAlice::NHollywood::NGeneralConversation {

void TGeneralConversationPrepareCommitSocialSharingLinkHandle::Do(TScenarioHandleContext& ctx) const {
    const auto createCandidateResponse =
        GetOnlyProtoOrThrow<TCreateCandidateResponse>(ctx.ServiceCtx, SOCIAL_SHARING_LINK_CREATE_RESPONSE_ITEM);

    if (createCandidateResponse.HasLink()) {
        TCommitCandidateRequest request;
        request.SetCandidateId(createCandidateResponse.GetLink().GetId());
        ctx.ServiceCtx.AddProtobufItem(request, SOCIAL_SHARING_LINK_COMMIT_REQUEST_ITEM);
    }
}

}  // namespace NAlice::NHollywood::NGeneralConversation
