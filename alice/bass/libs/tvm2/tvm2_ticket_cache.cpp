#include "tvm2_ticket_cache.h"

#include <alice/bass/libs/config/config.h>

namespace NBASS {

namespace {
TVector<TString> GetServices(const TConfig& config) {
    TVector<TString> ids;
    for (const auto& kv : config.Vins()->GetRawValue()->GetDict()) {
        const TStringBuf id{kv.second["Tvm2ClientId"].GetString()};
        if (!id.empty())
            ids.push_back(TString{id});
    }
    return ids;
}
} // namespace

// TTVM2TicketCache::TDelegate ------------------------------------------
TTVM2TicketCache::TDelegate::TDelegate(const TSourcesRegistry& sources, const TConfig& config,
                                       const IGlobalContext::TSecrets& secrets)
    : ClientId(TString{*config.Tvm2().BassTvm2ClientId()})
    , ClientSecret(secrets.TVM2Secret)
    , Tvm2SourceContext(sources.Tvm2)
    , SourceRequestFactory(Tvm2SourceContext, config, TStringBuf("/2/ticket"), SourcesRegistryDelegate) {
}

// TTVM2TicketCache -----------------------------------------------------
TTVM2TicketCache::TTVM2TicketCache(const TSourcesRegistry& sources, const TConfig& config,
                                   const IGlobalContext::TSecrets& secrets)
    : Delegate(sources, config, secrets)
    , TicketCache(Delegate, GetServices(config)) {
}
} // namespace NBASS
