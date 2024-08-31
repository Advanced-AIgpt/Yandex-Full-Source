#pragma once

#include <alice/megamind/library/config/protos/config.pb.h>

#include <alice/megamind/library/sources/request.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NSaasSearch {

TSourcePrepareStatus PrepareSaasRequest(const TString& normalizedRequestText,
                                        const TConfig::TSaasSourceOptions& saasConfig,
                                        NNetwork::IRequestBuilder& request);

NScenarios::TSkillDiscoverySaasCandidates ParseSaasSkillDiscoveryReply(const TString& content,
                                                                       const TConfig::TSaasSourceOptions& saasConfig);

} // namespace NAlice::NSaasSearch
