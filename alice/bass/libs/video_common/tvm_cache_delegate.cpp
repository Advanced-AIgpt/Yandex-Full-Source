#include "tvm_cache_delegate.h"

#include <alice/bass/libs/logging_v2/logger.h>

namespace NVideoCommon {

THolder<NHttpFetcher::TRequest> TTicketCacheDelegate::MakeRequest() {
    NHttpFetcher::TRequestOptions options;
    options.Timeout = TDuration::Seconds(10);
    options.MaxAttempts = 3;
    options.RetryPeriod = TDuration::Seconds(3);
    return NHttpFetcher::Request(TvmApiUrl, options);
}

TMaybe<TString> GetSingleTvm2Ticket(TStringBuf tvm2Id, TStringBuf tvm2Secret, TStringBuf tvm2ServiceId,
                                    TStringBuf tvmApiUrl) {
    TTicketCacheDelegate cacheDelegate{tvm2Id, tvm2Secret, tvmApiUrl};
    NTVM2::TTicketCacheHolder ticketCache{cacheDelegate};
    ticketCache.AddService(tvm2ServiceId);

    if (ticketCache.Update())
        LOG(ERR) << "Failed to get TVM 2.0 tickets" << Endl;

    return ticketCache.GetTicket(tvm2ServiceId);
}

} // namespace NVideoCommon
