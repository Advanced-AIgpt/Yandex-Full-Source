#include "response.h"

#include <search/session/compression/report.h>

namespace NAlice::NKvSaaS {

TStatus IResponse::Parse(const TString& data) {
    NMetaProtocol::TReport report;
    if (!report.ParseFromString(data)) {
        return TError() << "KvSasS: Could not parse response data";
    }
    NMetaProtocol::Decompress(report);

    for (size_t i = 0; i < report.GroupingSize(); ++i) {
        const NMetaProtocol::TGrouping& grouping = report.GetGrouping(i);
        for (size_t j = 0; j < grouping.GroupSize(); ++j) {
            const NMetaProtocol::TGroup& group = grouping.GetGroup(j);
            for (size_t k = 0; k < group.DocumentSize(); ++k) {
                const NMetaProtocol::TDocument& document = group.GetDocument(k);
                const NMetaProtocol::TArchiveInfo& archiveInfo = document.GetArchiveInfo();
                for (size_t g = 0; g < archiveInfo.GtaRelatedAttributeSize(); ++g) {
                    const NMetaProtocol::TPairBytesBytes& gta = archiveInfo.GetGtaRelatedAttribute(g);
                    if (auto error = ParseMessage(archiveInfo.GetTitle(), gta.GetKey(), gta.GetValue())) {
                        return error;
                    }
                }
            }
        }
    }

    return ParsingStatus();
}

TStatus TPersonalIntentsResponse::ParseMessage(const TString& /*url*/, const TString& key, const TString& value) {
    if (TStringBuf("PersonalIntents") == key) {
        TPersonalIntentsRecord::TPersonalIntents personalIntents;
        if (personalIntents.ParseFromString(value)) {
            ParsedResponse.ConstructInPlace(std::move(personalIntents));
            return Success();
        }
        return TError() << "KvSasS: Could not parse PersonalIntents record";
    }

    return Success();
}

TStatus TPersonalIntentsResponse::ParsingStatus() {
    if (!ParsedResponse.Defined()) {
        return TError() << "KvSasS: PersonalIntents record not found";
    }

    return Success();
}

TStatus TTokensStatsResponse::ParseMessage(const TString& url, const TString& key, const TString& value) {
    if (TStringBuf("ClientsIntentsStat") == key) {
        TTokenStatsByClients tokenStats;
        tokenStats.Token = url;
        if (tokenStats.ClientsStats.ParseFromString(value)) {
            TokensStatsByClients.emplace_back(std::move(tokenStats));
            return Success();
        }
        return TError() << "KvSasS: Could not parse token stats record";
    }

    return Success();
}

} // namespace NAlice::NKvSaaS
