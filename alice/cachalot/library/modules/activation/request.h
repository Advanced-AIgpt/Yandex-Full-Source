#pragma once

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/activation/ana_log.h>
#include <alice/cachalot/library/modules/activation/common.h>
#include <alice/cachalot/library/modules/activation/storage.h>
#include <alice/cachalot/library/request.h>

#include <apphost/api/service/cpp/service_context.h>


namespace NCachalot {


class TRequestActivationBase : public TRequest {
protected:
    TRequestActivationBase(
        NAppHost::TServiceContextPtr ctx,
        TIntrusivePtr<IActivationStorage> storage,
        TActivationServiceConfig config
    );

public:
    TRequestActivationBase(
        const NNeh::IRequestRef& req,
        TIntrusivePtr<IActivationStorage> storage,
        TActivationServiceConfig config
    );

protected:
    TIntrusivePtr<IActivationStorage> Storage;
    TActivationAlgorithmMetrics* ActivationAlgorithmMetrics;
    TActivationServiceConfig ServiceConfig;
    TActivationStorageRequestOptions Options;
    TActivationAnaLog AnaLog;
};


class TRequestActivationAnnouncement : public TRequestActivationBase {
public:
    using TRequestActivationBase::TRequestActivationBase;

    TAsyncStatus ServeAsync() override;

protected:
    TIntrusivePtr<TRequestActivationAnnouncement> IntrusiveThis() {
        return this;
    }
};


class TRequestVoiceInputActivationFirstAnnouncement : public TRequestActivationAnnouncement {
public:
    TRequestVoiceInputActivationFirstAnnouncement(
        NAppHost::TServiceContextPtr ctx,
        TIntrusivePtr<IActivationStorage> storage,
        TActivationServiceConfig config
    );

protected:
    void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) override;
    void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) override;
};


class TRequestVoiceInputActivationSecondAnnouncement : public TRequestActivationAnnouncement {
public:
    TRequestVoiceInputActivationSecondAnnouncement(
        NAppHost::TServiceContextPtr ctx,
        TIntrusivePtr<IActivationStorage> storage,
        TActivationServiceConfig config
    );

protected:
    void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) override;
    void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) override;
};


class TRequestActivationFinal : public TRequestActivationBase {
public:
    using TRequestActivationBase::TRequestActivationBase;

    TAsyncStatus ServeAsync() override;

protected:
    void MakeTryAcquireLeadershipRequest(
        NThreading::TPromise<TStatus> status,
        const TActivationStorageKey& requestKey,
        const TActivationStorageData& requestData,
        const TActivationStorageRequestOptions& options
    );

    TIntrusivePtr<TRequestActivationFinal> IntrusiveThis() {
        return this;
    }

    void UpdateAnaLogSpotterValidatedBy(const TSpotterValidationDevice& svd);
};


class TRequestVoiceInputActivationFinal : public TRequestActivationFinal {
public:
    TRequestVoiceInputActivationFinal(
        NAppHost::TServiceContextPtr ctx,
        TIntrusivePtr<IActivationStorage> storage,
        TActivationServiceConfig config
    );

protected:
    void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) override;
    void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) override;
};

}   // namespace NCachalot
