#include <alice/cachalot/library/modules/stats/service.h>

#include <alice/cachalot/library/golovan.h>


namespace NCachalot {


TMaybe<TStatsServiceSettings> TStatsService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.Stats();
}

bool TStatsService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, "/unistat", [](const NNeh::IRequestRef& request) {
        NNeh::TRequestOut response(request.Get());
        response << TUnistat::Instance().CreateJsonDump(0, true);
    });
    return true;
}


}   // namespace NCachalot
