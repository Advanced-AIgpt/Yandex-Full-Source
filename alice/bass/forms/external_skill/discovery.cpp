#include "discovery.h"

#include "dj_entry_point.h"
#include "fwd.h"
#include "skill.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/source_request/handle.h>

#include <alice/library/skill_discovery/common.h>

namespace NBASS::NExternalSkill {
namespace {

const TUtf16String ALICE_NAME = u"алиса";

} // namespace

// TRemoveAliceNameTokenHandler -----------------------------------------------------

void TRemoveAliceNameTokenHandler::OnToken(const TWideToken& token, size_t, NLP_TYPE type) {
    if (type == NLP_WORD) {
        TUtf16String normToken = ToLowerRet(TWtringBuf(token.Token, token.Leng));
        if (normToken == ALICE_NAME)
            return;
        HasWords_ = true;
    }
    Result += WideToUTF8(token.Text());
}

const TString& TRemoveAliceNameTokenHandler::GetResult() const {
    return Result;
}

bool TRemoveAliceNameTokenHandler::HasWords() const {
    return !HasWords_;
}

// TRemoveAliceNameTokenHandler -----------------------------------------------------

// TSkillBadge ----------------------------------------------------------------------

NSc::TValue TSkillBadge::ToJson() const {
    NSc::TValue v;
    v["id"] = Id;
    v["name"] = Name;
    v["description"] = Description;
    v["logo"] = AvatarUrl;
    v["url"] = Url;
    v["example"] = ActivationPhrase;
    return v;
}

// TSkillBadge ----------------------------------------------------------------------

NHttpFetcher::THandle::TRef FetchSkillsDiscoveryRequest(const TStringBuf query, TContext& context, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    TContext::TSlot* slotActivation = context.GetSlot(ACTIVATION_SLOT_NAME);
    auto requestCardName = IsSlotEmpty(slotActivation)
                           ? EServiceRequestCard::DiscoveryBASSSearch
                           : EServiceRequestCard::DiscoveryBASSUnknown;

    auto request = PrepareRequest(context, requestCardName, multiRequest);
    request->AddCgiParam("utterance", query);

    LOG(DEBUG) << "SkillsDiscoveryRequest: " << request->Url() << Endl;
    return request->Fetch();
}

TVector<const TSkillBadge> WaitDiscoveryResponses(IRequestHandle<TServiceResponse>* skillsRequestHandle, TContext& context) {
    if (!skillsRequestHandle) {
        return {};
    }

    TServiceResponse response;
    if (TResultValue err = skillsRequestHandle->WaitAndParseResponse(&response)) {
        return {};
    }

    if (response->Items().Empty()) {
        Y_STATS_INC_COUNTER_IF(!context.IsTestUser(), "skills_discovery_recommender_empty");
        LOG(DEBUG) << "nothing found" << Endl;
        return {};
    }

    TVector<const TSkillBadge> skillBadges;
    for (const auto& item : response->Items()) {
        TSkillBadge badge;

        badge.Id = item->Id();
        badge.Name = item->Activation();
        badge.Description = item->Description();
        badge.AvatarUrl = ConstructLogoUrl(item->Look(), item->LogoPrefix(), item->LogoAvatarId(), context);
        badge.Url = TClientActionUrl::OpenDialogByText(item->Activation());
        badge.ActivationPhrase = item->Activation();

        skillBadges.push_back(std::move(badge));
    }

    Y_STATS_INC_COUNTER_IF(!context.IsTestUser() && !skillBadges.empty(), "skills_discovery_recommender_found");

    return skillBadges;
}

} // namespace NBASS::NExternalSkill

