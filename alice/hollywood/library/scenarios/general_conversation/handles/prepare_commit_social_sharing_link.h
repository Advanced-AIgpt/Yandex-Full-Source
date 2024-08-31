#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TGeneralConversationPrepareCommitSocialSharingLinkHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare_commit_social_sharing_link";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NGeneralConversation
