#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/request.h>

#include <util/generic/fwd.h>
#include <util/generic/hash.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TSearchParams {
public:
    TSearchParams(const TString& context, const THashMap<TString, TString>& params);

public:
    void SetContext(const TString& context);
    void SetParam(const TString& key, const TString& value);
    void Patch(const TExpFlags& expFlags, const TStringBuf& prefix);
    TString ToString() const;

private:
    TString Context;
    THashMap<TString, TString> Params;
};

TSearchParams ConstructReplyCandidatesParams(TGeneralConversationRunContextWrapper& contextWrapper, const TString& context,
                                             const TSessionState& sessionState, const TClassificationResult& classificationResult);
TSearchParams ConstructSuggestCandidatesParams(const TString& context, const TScenarioRunRequestWrapper& requestWrapper);

} // namespace NAlice::NHollywood::NGeneralConversation
