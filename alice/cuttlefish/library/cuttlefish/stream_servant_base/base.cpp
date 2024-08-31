#include "base.h"

namespace NAlice::NCuttlefish::NAppHostServices {

TStreamServantBase::TStreamServantBase(
    NAppHost::TServiceContextPtr ctx,
    TLogContext logContext,
    TStringBuf sourceName
)
    : AhContext_(ctx)
    , Promise_(NThreading::NewPromise())
    , LogContext_(logContext)
    , Metrics_(*ctx, sourceName)
    , StartTime_(TInstant::Now())
    , IsFirstChunk_(true)
{}

void TStreamServantBase::OnNextInput() {
    if (IsFirstChunk_) {
        if (!ProcessFirstChunk()) {
            return;
        }
        IsFirstChunk_ = false;
    }

    if (!ProcessInput()) {
        return;
    }

    if (IsCompleted()) {
        // No more input items needed
        OnCompleted();
    } else {
        // Continue consuming input
        SubscribeToNextInput();
    }
}

NThreading::TPromise<void> TStreamServantBase::GetFinishPromise() {
    return Promise_;
}

void TStreamServantBase::SubscribeToNextInput() {
    TIntrusivePtr<TStreamServantBase> self(this);
    AhContext_->NextInput().Apply([stream = std::move(self)](auto hasData) mutable {
        if (hasData.GetValue()) {
            // Process new input items
            stream->OnNextInput();
        } else {
            // Finish processing
            stream->OnEndOfInputStream();
        }
    });
}

void TStreamServantBase::OnEndOfInputStream() {
    if (!IsCompleted()) {
        // Sometimes it is ok (cancel by user for example), but we alway want to know when it's happend
        // If it's fatal error for request code after this if will report this correctly
        Metrics_.PushRate("incompleteinput", "warning", "apphost");
    }

    if (AhContext_->IsTimedOut()) {
        OnTimedOut();
    } else if (AhContext_->IsCancelled()) {
        OnCancelled();
    } else {
        if (IsCompleted()) {
            OnCompleted();
        } else {
            OnIncompleteInput();
        }
    }
}

void TStreamServantBase::OnError(const TString& error, bool isCritical) {
    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << (isCritical ? "Critical error: " : "Error: ") << error);

    if (isCritical) {
        Promise_.SetException(error);
    }
}

void TStreamServantBase::OnCompleted() {
    LogContext_.LogEventInfoCombo<NEvClass::TAppHostEmptyInput>();

    AhContext_->Flush();
    Promise_.SetValue();
}

void TStreamServantBase::OnIncompleteInput() {
    Metrics_.SetError("incompleteinput");
    OnError(GetErrorForIncompleteInput(), /* isCritical = */ true);
}

void TStreamServantBase::OnTimedOut() {
    Metrics_.SetError("timeout");
    LogContext_.LogEventInfoCombo<NEvClass::RequestTimedOut>();

    Promise_.SetException("Request timed out");
}

void TStreamServantBase::OnCancelled() {
    Metrics_.PushRate("cancel", "warning", "apphost");
    LogContext_.LogEventInfoCombo<NEvClass::CancelRequest>();

    Promise_.SetValue();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
