#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class TStreamServantBase : public TThrRefBase {
public:
    TStreamServantBase(
        NAppHost::TServiceContextPtr ctx,
        TLogContext logContext,
        TStringBuf sourceName
    );

    void OnNextInput();
    NThreading::TPromise<void> GetFinishPromise();

protected:
    virtual bool ProcessFirstChunk() = 0;
    virtual bool ProcessInput() = 0;

    virtual bool IsCompleted() = 0;
    virtual TString GetErrorForIncompleteInput() = 0;

    virtual void OnCompleted();
    virtual void OnIncompleteInput();
    virtual void OnTimedOut();
    virtual void OnCancelled();

    virtual void OnError(const TString& error, bool isCritical = false);

private:
    void SubscribeToNextInput();
    void OnEndOfInputStream();

protected:
    NAppHost::TServiceContextPtr AhContext_;
    NThreading::TPromise<void> Promise_;

    TLogContext LogContext_;
    TSourceMetrics Metrics_;

    TInstant StartTime_;
    bool IsFirstChunk_;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
