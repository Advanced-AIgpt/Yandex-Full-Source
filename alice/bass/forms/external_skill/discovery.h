#pragma once

#include "fwd.h"

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/source_request/handle.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBASS::NExternalSkill {

inline constexpr TStringBuf ACTIVATION_SLOT_NAME = "activation";

struct TSkillBadge {
    TString Id;
    TString Name;
    TString Description;
    TString AvatarUrl;
    TString Url;
    TString ActivationPhrase;

    NSc::TValue ToJson() const;
};

class TRemoveAliceNameTokenHandler : public ITokenHandler {
private:
    TString Result;

    bool HasWords_ = false;

public:
    void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override;

    bool HasWords() const;

    const TString& GetResult() const;
};

NHttpFetcher::THandle::TRef FetchSkillsDiscoveryRequest(TStringBuf query, TContext& context, NHttpFetcher::IMultiRequest::TRef multiRequest);
TVector<const TSkillBadge> WaitDiscoveryResponses(IRequestHandle<TServiceResponse>* skillsRequestHandler, TContext& context);

} // namespace NBASS::NExternalSkill
