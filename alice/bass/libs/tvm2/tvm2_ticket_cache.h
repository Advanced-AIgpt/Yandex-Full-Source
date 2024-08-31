#pragma once

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/libs/tvm2/ticket_cache/ticket_cache.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS {

class ITVM2TicketCache : NNonCopyable::TNonCopyable {
public:
    virtual ~ITVM2TicketCache() = default;

    virtual bool Update() = 0;

    virtual TMaybe<TString> GetTicket(TStringBuf serviceId) = 0;

    virtual TDuration SinceLastUpdate() = 0;
};

class TTVM2TicketCache : public ITVM2TicketCache {
public:
    TTVM2TicketCache(const TSourcesRegistry& sources, const TConfig& config, const IGlobalContext::TSecrets& secrets);

    bool Update() override {
        if (const auto error = TicketCache.Update()) {
            LOG(ERR) << "Failed to update TVM2 ticket cache: " << *error << Endl;
            return false;
        }
        return true;
    }

    TMaybe<TString> GetTicket(TStringBuf serviceId) override {
        return TicketCache.GetTicket(serviceId);
    }

    TDuration SinceLastUpdate() override {
        return TicketCache.SinceLastUpdate();
    }

private:
    class TDelegate : public NTVM2::TTicketCacheHolder::IDelegate {
    public:
        TDelegate(const TSourcesRegistry& sources, const TConfig& config, const IGlobalContext::TSecrets& secrets);

        // NTVM2::TTicketCache::IDelegate overrides:
        TString GetClientId() override {
            return ClientId;
        }

        TString GetClientSecret() override {
            return ClientSecret;
        }

        THolder<NHttpFetcher::TRequest> MakeRequest() override {
            return SourceRequestFactory.Request();
        }

    private:
        const TString ClientId;
        const TString ClientSecret;

        const TSourceContext& Tvm2SourceContext;
        TDummySourcesRegistryDelegate SourcesRegistryDelegate;
        const TSourceRequestFactory SourceRequestFactory;
    };

private:
    TDelegate Delegate;
    NTVM2::TTicketCache TicketCache;
};

class TFakeTvm2TicketCache : public ITVM2TicketCache {
public:
    bool Update() override {
        return true;
    }

    TMaybe<TString> GetTicket(TStringBuf /*serviceId*/) override {
        return "mockedTmv";
    }

    TDuration SinceLastUpdate() override {
        return TDuration::Seconds(5);
    }
};

} // namespace NBASS
