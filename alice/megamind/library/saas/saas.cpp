#include "saas.h"

#include <alice/library/skill_discovery/common.h>

#include <saas/api/search_client/client.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NAlice::NSaasSearch {

TSourcePrepareStatus PrepareSaasRequest(const TString& normalizedRequestText,
                                        const TConfig::TSaasSourceOptions& saasSourceOptions,
                                        NNetwork::IRequestBuilder& request) {
    if (normalizedRequestText.empty()) {
        return TError{TError::EType::Empty} << "Empty utterance to call Saas";
    }

    const auto& saasQueryParams = saasSourceOptions.GetSaasQueryParams();
    const auto& host = saasSourceOptions.GetHost();
    const auto& port = saasSourceOptions.GetPort();

    NSaas::TQuery query;
    TStringStream queryText, relev, extraParams;
    queryText << normalizedRequestText << " softness:" << saasQueryParams.GetSoftness();
    relev << "formula=" << saasQueryParams.GetFormula();

    const auto& how = saasQueryParams.GetHow();
    if (!how.empty()) {
        extraParams << "&how=" << how;
    }
    extraParams << "&relev=" << relev.Str();
    query.SetKps(saasQueryParams.GetKps());
    query.SetExtraParams(extraParams.Str());
    query.SetText(CGIEscapeRet(queryText.Str()));
    NSaas::TSearchClient client{saasSourceOptions.GetServiceName(), TString{host}, TIpPort(port)};
    TCgiParameters cgis(client.GetFullQueryString(query, TDuration::MilliSeconds(saasSourceOptions.GetTimeoutMs())));
    request.AddCgiParams(cgis);
    return ESourcePrepareType::Succeeded;
}

NScenarios::TSkillDiscoverySaasCandidates ParseSaasSkillDiscoveryReply(const TString& content,
                                                                       const TConfig::TSaasSourceOptions& saasSourceOptions) {
    auto reply = NSaas::TSearchReply::FromProtoReply(0, content.Data(), content.size());
    TVector<TString> skillIds;
    THashMap<TString, double> id2Relev;
    NScenarios::TSkillDiscoverySaasCandidates result;

    const auto& threshold = saasSourceOptions.GetSaasQueryParams().GetThreshold();
    int candidateCount = 0;
    int documentsFound = 0;
    reply.ScanDocs([&skillIds, &id2Relev, threshold, &candidateCount, &documentsFound](const NMetaProtocol::TDocument& doc) {
        documentsFound++;
        TStringBuf skillId = doc.GetArchiveInfo().GetUrl();
        double relev = doc.GetRelevance();
        if (relev > threshold) {
            candidateCount++;
            skillIds.emplace_back(skillId);
            id2Relev.emplace(skillId, NAlice::NSkillDiscovery::NormalizeRelev(relev));
        }
    });
    SortBy(skillIds.begin(), skillIds.end(), [&id2Relev] (const auto& id) { return -id2Relev.at(id); });
    for (const auto& id : skillIds) {
        auto& candidate = *result.AddSaasCandidate();
        candidate.SetSkillId(id);
        candidate.SetRelevance(id2Relev.at(id));
    }
    return result;
}

} // namespace NAlice::NSaasSearch
