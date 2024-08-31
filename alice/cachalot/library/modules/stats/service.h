#pragma once

#include <alice/cachalot/library/service.h>


namespace NCachalot {


class TStatsService : public IService {
public:
    static TMaybe<TStatsServiceSettings> FindServiceSettings(const TApplicationSettings&);

public:
    explicit TStatsService(const TStatsServiceSettings&) {};
    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;
};


}   // namespace NCachalot
