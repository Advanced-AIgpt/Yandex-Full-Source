#pragma once

#include "client.h"
#include "response.h"
#include <alice/hollywood/library/scenarios/market/common/context.h>
#include <alice/hollywood/library/scenarios/market/common/search_info.h>

namespace NAlice::NHollywood::NMarket {

/*
    Методы для отправления/получения репорт-запросов через app_host
*/
void AddSearchInfoRequest(
    const TSearchInfo& searchInfo,
    TReportClient& client,
    TMarketBaseContext& ctx,
    bool allowRedirects);
void AddSearchInfo(const TSearchInfo& searchInfo, TMarketBaseContext& ctx);
void AddReportClient(const TReportClient& client, TMarketBaseContext& ctx);
void AddReportRequest(const TReportPrimeRequest& request, TMarketBaseContext& ctx);
void AddReportClient(const TReportClient& client, TMarketBaseContext& ctx);

TSearchInfo GetSearchInfoOrThrow(const TMarketBaseContext& ctx);
TReportClient GetReportClientOrThrow(const TMarketBaseContext& ctx);
TReportPrimeResponse RetireReportPrimeResponse(const TMarketBaseContext& ctx);

} // namespace NAlice::NHollywood::NMarket
