#pragma once

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/tvm2/ticket_cache/ticket_cache.h>

namespace NVideoCommon {

inline constexpr TStringBuf TVM_API_URL = "https://tvm-api.yandex.net/2/ticket";

class TTicketCacheDelegate : public NTVM2::TTicketCache::IDelegate {
public:
    TTicketCacheDelegate(TStringBuf tvm2Id, TStringBuf tvm2Secret, TStringBuf tvmApiUrl = TVM_API_URL)
        : Id{TString{tvm2Id}}
        , Secret{TString{tvm2Secret}}
        , TvmApiUrl{TString{tvmApiUrl}} {
    }

    // NTVM2::TTicketCache::IDelegate overrides:
    TString GetClientId() override {
        return Id;
    }

    TString GetClientSecret() override {
        return Secret;
    }

    THolder<NHttpFetcher::TRequest> MakeRequest() override;

private:
    const TString Id;
    const TString Secret;
    const TString TvmApiUrl;
};

TMaybe<TString> GetSingleTvm2Ticket(TStringBuf tvm2Id, TStringBuf tvm2Secret, TStringBuf tvm2ServiceId,
                                    TStringBuf tvmApiUrl = TVM_API_URL);

} // namespace NVideoCommon
