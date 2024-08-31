#pragma once

#include "polyglot_translate_utterance_response.h"
#include "wizard_response.h"

#include <alice/library/blackbox/proto/blackbox.pb.h>

#include <alice/megamind/library/apphost_request/protos/begemot_response_parts.pb.h>
#include <alice/megamind/library/apphost_request/protos/web_search_query.pb.h>
#include <alice/megamind/library/entity_search/response.h>
#include <alice/megamind/library/kv_saas/response.h>
#include <alice/megamind/library/search/search.h>
#include <alice/megamind/library/session/protos/session.pb.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/protos/common/misspell.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NDJ::NAS {

class TProactivityResponse;

} // namespace NDJ::NAS

namespace NAlice {

class IResponses {
public:
    virtual ~IResponses() = default;

    virtual const TPolyglotTranslateUtteranceResponse& PolyglotTranslateUtteranceResponse(TStatus* status = nullptr) const = 0;
    virtual const TBlackBoxFullUserInfoProto& BlackBoxResponse(TStatus* status = nullptr) const = 0;
    virtual const TEntitySearchResponse& EntitySearchResponse(TStatus* status = nullptr) const = 0;
    virtual const TMisspellProto& MisspellResponse(TStatus* status = nullptr) const = 0;
    virtual const NKvSaaS::TPersonalIntentsResponse& PersonalIntentsResponse(TStatus* status = nullptr) const = 0;
    virtual const NKvSaaS::TTokensStatsResponse& QueryTokensStatsResponse(TStatus* status = nullptr) const = 0;
    virtual const NScenarios::TSkillDiscoverySaasCandidates& SaasSkillDiscoveryResponse(TStatus* status = nullptr) const = 0;
    virtual const TWizardResponse& WizardResponse(TStatus* status = nullptr) const = 0;
    virtual const NDJ::NAS::TProactivityResponse& ProactivityResponse(TStatus* status = nullptr) const = 0;
    virtual const TSearchResponse& WebSearchResponse(TStatus* status = nullptr) const = 0;
    virtual const NMegamindAppHost::TWebSearchQueryProto& WebSearchQueryResponse(TStatus* status = nullptr) const = 0;
    virtual const NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest& BegemotResponseRewrittenRequestResponse(TStatus* status = nullptr) const = 0;
    virtual const TSessionProto& SpeechKitSessionResponse(TStatus* status = nullptr) const = 0;
};

} // namespace NAlice
