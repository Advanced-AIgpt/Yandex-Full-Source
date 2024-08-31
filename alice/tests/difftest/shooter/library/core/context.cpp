#include "context.h"

#include <alice/joker/library/log/log.h>

#include <infra/yp_service_discovery/resolver/resolver.h>

#include <util/generic/guid.h>

using namespace NServiceDiscovery;
using namespace NYP::NServiceDiscovery;

namespace NAlice::NShooter {

namespace {

TString GetHostName() {
    char hostnameBuf[1024];
    gethostname(hostnameBuf, 1024);
    return hostnameBuf;
}

// https://wiki.yandex-team.ru/yp/discovery/usage/
std::pair<TString, ui16> DiscoverEndpointHostPort(TConfig::TNannyJokerServerConst nanny) {
    TString hostName = GetHostName();
    LOG(INFO) << "My hostname: " << hostName.Quote() << Endl;

    LOG(INFO) << "YP Discovery Joker endpoint host & port" << Endl;
    NApi::TReqResolveEndpoints request;
    request.set_cluster_name(TString{nanny.ClusterName()});
    request.set_endpoint_set_id(TString{nanny.EndpointSetId()});

    const auto& response = TResolver(std::move(hostName)).Resolve(request);
    LOG(INFO) << "YP Discovery status: " << static_cast<int>(response.resolve_status()) << Endl;
    Y_ASSERT(response.resolve_status() == NApi::EResolveStatus::OK);

    const auto& endpoint = response.endpoint_set().endpoints(0);
    return {endpoint.fqdn(), endpoint.port()};
}

TMaybe<TJokerServerSettings> ConstructJokerServerSettings(const TConfig& config) {
    if (!config.HasJokerConfig()) {
        return Nothing();
    }

    const auto& jokerConfig = config.JokerConfig();
    const auto& type = jokerConfig.ServerType();

    TJokerServerSettings settings;
    if (type == "plain") {
        TConfig::TPlainJokerServerConst plain(jokerConfig.Server());
        settings.Host = plain.Host();
        settings.Port = plain.Port();
    } else if (type == "nanny") {
        TConfig::TNannyJokerServerConst nanny(jokerConfig.Server());
        auto [host, port] = DiscoverEndpointHostPort(nanny);
        settings.Host = host;
        settings.Port = port;
    }

    LOG(INFO) << "Joker host " << settings.Host.Quote() << ", port " << settings.Port << Endl;
    return settings;
}

} // namespace

TContext::TContext(const TFsPath& configFilePath, TTokens tokens)
    : Config_{configFilePath}
    , Tokens_{std::move(tokens)}
    , Yav_{MakeHolder<TYav>(TString{Config_.YavSecretId()}, Tokens_.YavToken)}
    , JokerServerSettings_{ConstructJokerServerSettings(Config_)}
{
}

} // namespace NAlice::NShooter
