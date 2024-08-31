#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_REQUEST_NAME =
    "mm_saas_skill_discovery_http_request";
inline constexpr TStringBuf AH_ITEM_SAAS_SKILL_DISCOVERY_HTTP_RESPONSE_NAME =
    "mm_saas_skill_discovery_http_response";

TStatus AppHostSaasSkillDiscoverySetup(IAppHostCtx& ahCtx, const TString& utterance);
TStatus AppHostSaasSkillDiscoveryPostSetup(IAppHostCtx& ahCtx,
                                           NScenarios::TSkillDiscoverySaasCandidates& protoResponse);

} // namespace NAlice::NMegamind
