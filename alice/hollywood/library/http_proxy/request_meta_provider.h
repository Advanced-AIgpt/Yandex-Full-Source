#pragma once

#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood {

class IRequestMetaProvider {
public:
    virtual ~IRequestMetaProvider() = default;

    virtual const TString& GetRequestId() const = 0;
    virtual const TString& GetClientIP() const = 0;
    virtual const TString& GetOAuthToken() const = 0;
    virtual const TString& GetUserTicket() const = 0;
};

class TRequestMetaProvider : public IRequestMetaProvider {
public:
    explicit TRequestMetaProvider(const NScenarios::TRequestMeta& meta);

    const TString& GetRequestId() const override;
    const TString& GetClientIP() const override;
    const TString& GetOAuthToken() const override;
    const TString& GetUserTicket() const override;

private:
    const NScenarios::TRequestMeta& Meta_;
};

} // namesapce NAlice::NHollywood
