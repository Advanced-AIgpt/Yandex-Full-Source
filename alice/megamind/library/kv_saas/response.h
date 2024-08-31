#pragma once

#include <alice/megamind/library/util/status.h>
#include <alice/library/intent_stats/proto/intent_stats.pb.h>
#include <alice/quality/user_intents/proto/personal_intents.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/system/types.h>

namespace NAlice::NKvSaaS {

class IResponse {
public:
    virtual ~IResponse() = default;

    [[nodiscard]] TStatus Parse(const TString& data);
private:
    [[nodiscard]] virtual TStatus ParseMessage(const TString& url, const TString& key, const TString& value) = 0;

    virtual TStatus ParsingStatus() {
        return Success();
    }
};

class TPersonalIntentsResponse final : public IResponse {
public:
    TPersonalIntentsResponse() = default;

    // for unit-test
    explicit TPersonalIntentsResponse(const TPersonalIntentsRecord::TPersonalIntents& parsed)
        : ParsedResponse(parsed)
    {
    }

    const TMaybe<TPersonalIntentsRecord::TPersonalIntents>& ProtoResponse() const {
        return ParsedResponse;
    }

private:
    [[nodiscard]] TStatus ParseMessage(const TString& url, const TString& key, const TString& value) override;

    TStatus ParsingStatus() override;

private:
    TMaybe<TPersonalIntentsRecord::TPersonalIntents> ParsedResponse;
};

class TTokensStatsResponse final : public IResponse {
public:
    struct TTokenStatsByClients {
        TString Token;
        TIntentsStatRecord::TClientsIntentsStat ClientsStats;
    };

    TTokensStatsResponse() = default;

    const TVector<TTokenStatsByClients>& GetTokensStatsByClients() const {
        return TokensStatsByClients;
    }

private:
    [[nodiscard]] TStatus ParseMessage(const TString& url, const TString& key, const TString& value) override;

private:
    TVector<TTokenStatsByClients> TokensStatsByClients;
};

} // namespace NAlice::NKvSaaS
