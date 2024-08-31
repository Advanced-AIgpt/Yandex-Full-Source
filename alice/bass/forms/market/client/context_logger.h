#pragma once

#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/context.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace NMarket {

class TMarketClientCtxLogger: public TMarketClientLogger {
public:
    explicit TMarketClientCtxLogger(TMarketContext& ctx)
        : Ctx(ctx)
    {
    }

    void Log(const TStringBuf msg) override
    {
        LOG(DEBUG) << msg << Endl;
        Ctx.Log(msg);
    }

private:
    TMarketContext& Ctx;
};

} // namespace NMarket

} // namespace NBASS
