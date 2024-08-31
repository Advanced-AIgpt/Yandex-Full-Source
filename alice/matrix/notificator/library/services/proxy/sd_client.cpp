#include "sd_client.h"

#include <util/generic/maybe.h>
#include <util/network/address.h>
#include <util/random/random.h>
#include <util/system/rwlock.h>


namespace NMatrix::NNotificator {

TSDClientDummy::TSDClientDummy(const TString& address)
    : Address_(address)
{}

TMaybe<TSDClientDummy::TAddressInfo> TSDClientDummy::GetAddress() const {
    return TAddressInfo({
        .Address = Address_,
        .IsLocalDc = true,
    });
}

class TSDClient::TEndpointsProvider : public NYP::NServiceDiscovery::IEndpointSetProvider {
public:
    void Update(const NYP::NServiceDiscovery::TEndpointSetEx& endpointSet) override {
        TVector<TString> newAddresses;
        newAddresses.reserve(endpointSet.endpoints().size());
        for (const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint : endpointSet.endpoints()) {
            TNetworkAddress address(0);
            if (endpoint.ip6_address()) {
                address = TNetworkAddress(endpoint.ip6_address(), endpoint.port());
            } else {
                address = TNetworkAddress(endpoint.ip4_address(), endpoint.port());
            }
            newAddresses.emplace_back(
                TString::Join("http://", NAddr::PrintHostAndPort(NAddr::TAddrInfo(&*address.Begin())))
            );
        }

        {
            TWriteGuard guard(AddressesMutex_);
            Addresses_.swap(newAddresses);
        }
    }

    TMaybe<TString> GetRandomAddress() const {
        TReadGuard guard(AddressesMutex_);

        if (Y_UNLIKELY(Addresses_.empty())) {
            return Nothing();
        }

        return Addresses_[RandomNumber<size_t>(Addresses_.size())];
    }

private:
    TRWMutex AddressesMutex_;
    TVector<TString> Addresses_;
};

TSDClient::TSDClient(const TProxyServiceSettings::TSDSettings& config)
    : ServiceDiscoveryResolver_(MakeAtomicShared<NYP::NServiceDiscovery::TGrpcResolver>(config.GetSDConfig()))
    , EndpointSetManager_(NYP::NServiceDiscovery::TSDConfig(config.GetSDConfig()), ServiceDiscoveryResolver_)
{
    for (const auto& endpointSet : config.GetEndpointSetKeys()) {
        NYP::NServiceDiscovery::TEndpointSetKey endpointSetKey(endpointSet.GetCluster(), endpointSet.GetId());
        if (AsciiEqualsIgnoreCase(endpointSet.GetCluster(), config.GetLocalCluster())) {
            LocalEndpointsProviders_.emplace_back(EndpointSetManager_.RegisterEndpointSet<TEndpointsProvider>(std::move(endpointSetKey)));
        } else {
            EndpointsProviders_.emplace_back(EndpointSetManager_.RegisterEndpointSet<TEndpointsProvider>(std::move(endpointSetKey)));
        }
    }
    EndpointSetManager_.Start(FromString<TDuration>(config.GetSDConfig().GetUpdateFrequency()));
}

TMaybe<TSDClient::TAddressInfo> TSDClient::GetAddress() const {
    if (Y_LIKELY(!LocalEndpointsProviders_.empty())) {
        const auto address = LocalEndpointsProviders_[RandomNumber<size_t>(LocalEndpointsProviders_.size())]->GetRandomAddress();
        if (Y_LIKELY(address.Defined())) {
            return TAddressInfo({
                .Address = *address,
                .IsLocalDc = true,
            });
        }
    }

    if (Y_LIKELY(!EndpointsProviders_.empty())) {
        const auto address = EndpointsProviders_[RandomNumber<size_t>(EndpointsProviders_.size())]->GetRandomAddress();
        if (Y_LIKELY(address.Defined())) {
            return TAddressInfo({
                .Address = *address,
                .IsLocalDc = false,
            });
        }
    }

    return Nothing();
}

} // namespace NMatrix::NNotificator
