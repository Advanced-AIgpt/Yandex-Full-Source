#pragma once

#include <alice/matrix/notificator/library/config/config.pb.h>

#include <infra/yp_service_discovery/libs/sdlib/grpc_resolver/grpc_resolver.h>
#include <infra/yp_service_discovery/libs/sdlib/manager.h>

#include <util/generic/maybe.h>

namespace NMatrix::NNotificator {

class TSDClientBase {
public:
    struct TAddressInfo {
        const TString Address;
        const bool IsLocalDc;
    };

public:
    virtual TMaybe<TAddressInfo> GetAddress() const = 0;

    virtual ~TSDClientBase() = default;
};

class TSDClientDummy : public TSDClientBase {
public:
    TSDClientDummy(const TString& address);

    // IsLocalDc is always 'true'.
    TMaybe<TAddressInfo> GetAddress() const override;

private:
    TString Address_;
};

class TSDClient : public TSDClientBase {
public:
    TSDClient(const TProxyServiceSettings::TSDSettings& config);

    TMaybe<TAddressInfo> GetAddress() const override;

private:
    class TEndpointsProvider;

private:
    NYP::NServiceDiscovery::IRemoteRequesterPtr ServiceDiscoveryResolver_;
    NYP::NServiceDiscovery::TEndpointSetManager EndpointSetManager_;
    TVector<TEndpointsProvider*> LocalEndpointsProviders_;
    TVector<TEndpointsProvider*> EndpointsProviders_;
};

} // namespace NMatrix::NNotificator
