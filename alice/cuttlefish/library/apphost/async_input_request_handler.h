#pragma once

#include <alice/cuttlefish/library/logging/dlog.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/ptr.h>
#include <exception>


namespace NAlice::NCuttlefish {
    // guard for more safe work with async apphost request (helper for avoid race with read context&setting promise)
    class TInputAppHostAsyncRequestHandler : public TThrRefBase {
    public:
        TInputAppHostAsyncRequestHandler(NAppHost::TServiceContextPtr ctx, NThreading::TPromise<void> promise)
            : Ctx_(std::move(ctx))
            , Promise_(std::move(promise))
        {
        }

        const NThreading::TPromise<void> Promise() {
            return Promise_;
        }
        NAppHost::IServiceContext& Context() {
            return *Ctx_;
        }

        // WARNING: it DOES NOT support concurrent or recursive processing
        bool TryBeginProcessing();
        void EndProcessing();

        void Finish();
        void SetException(std::exception_ptr e);

    private:
        // state machine (we MUST NOT finish & process request simultaneously (in separate threads)):
        //
        // Idle  <------> Processing
        //   |                |
        //   v                v
        // Finished <---- NeedFinish
        //
        enum {
            Idle,
            Processing,
            NeedFinish,
            Finished,
        };
        TAtomic State_ = Idle;
        TAdaptiveLock ExceptionLock_;
        NAppHost::TServiceContextPtr Ctx_;
        NThreading::TPromise<void> Promise_;  // put here end request or exception
        std::exception_ptr Exception_;  // keep here exception for Promise_ (if can send it immediately)
    };

    using TInputAppHostAsyncRequestHandlerPtr = TIntrusivePtr<TInputAppHostAsyncRequestHandler>;


    class TContextGuard {  // behaves like AutoPtr
    public:
        TContextGuard() = default;
        TContextGuard(TInputAppHostAsyncRequestHandler& handler)
            : H_(&handler)
        {
            Y_ENSURE(H_->TryBeginProcessing());
        }

        TContextGuard(const TContextGuard& x) noexcept {
            operator=(x);
        }
        TContextGuard(TContextGuard&& x) noexcept {
            operator=(std::move(x));
        }

        ~TContextGuard() {
            if (H_) {
                H_->EndProcessing();
            }
        }

        TContextGuard& operator=(const TContextGuard& x) noexcept {
            return operator=(std::move(const_cast<TContextGuard&>(x)));
        }
        TContextGuard& operator=(TContextGuard&& x) noexcept {
            std::swap(H_, x.H_);
            return *this;
        }

    private:
        TInputAppHostAsyncRequestHandler* H_ = nullptr;
    };
}
