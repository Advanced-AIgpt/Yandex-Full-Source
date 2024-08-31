#include "http_requester.h"

#include <util/generic/list.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/system/tls.h>

#include <thread>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

class THttpRequester {
public:
    THttpRequester()
        : Client_(NNeh::CreateMultiClient())
        , Thread_([this](){
            TThread::SetCurrentThreadName("http-requester");
            while (true) {
                try {
                    Run();
                    return;
                } catch (const std::exception& exc) {
                    static const std::chrono::seconds SleepBeforeRestoreDuration{3};
                    Cerr <<
                        "thread epically crashed: " << exc.what() << "\n"
                        "it will be restarted in " << SleepBeforeRestoreDuration.count() << Endl;
                    std::this_thread::sleep_for(SleepBeforeRestoreDuration);
                }
            }
        })
    {}

    ~THttpRequester() {
        Client_->Interrupt();
        Thread_.join();
    }

    void SendRequest(const NNeh::TMessage& msg, const TDuration timeout, unsigned additionalAttempts, THttpRequestCallback&& callback, TLogContext logContext, int subrequestId) {
        if (!callback) {
            throw yexception() << "#" << subrequestId << " request with empty callback";
        }

        SendRequest(msg, MakeIntrusive<TRequestContext>(std::move(callback), std::move(logContext), timeout.ToDeadLine(), additionalAttempts, subrequestId));
    }

private:
    struct TRequestContext;
    using TRequestContextPtr = TIntrusivePtr<TRequestContext>;
    struct TRequestContext : public TThrRefBase {
        TRequestContext(THttpRequestCallback&& callback, TLogContext&& logContext, TInstant deadline, unsigned additionalAttempts, int subrequestId)
            : Callback(std::move(callback))
            , LogContext(std::move(logContext))
            , Deadline(deadline)
            , AdditionalAttempts(additionalAttempts)
            , SubrequestId(subrequestId)
        {}

        THttpRequestCallback Callback;
        TLogContext LogContext;
        TInstant Deadline;
        NNeh::TResponseRef FirstError;
        unsigned AdditionalAttempts;
        int SubrequestId;
        unsigned AttemptNumber{0};
        TList<TRequestContextPtr>::iterator GlobalRequestContextsListIterator;

        TString Id() const {
            TStringStream ss;
            ss << '#';
            if (SubrequestId >= 0) {
                ss << SubrequestId;
            }

            ss << '.' << AttemptNumber;
            return ss.Str();
        }

        void LogNextAttemptWarningCombo(TStringBuf errorType, int code, const TString& errorText) {
            TString messageNextAttemptAfterError{TStringBuilder() << Id() << " use next MM request attempt after error: "sv << errorType << "." << code << " " << errorText};
            LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(messageNextAttemptAfterError);
        }
    };

    void SendRequest(const NNeh::TMessage& msg, const TRequestContextPtr& ctx) {
        ctx->LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << ctx->Id() << " http request to '" << msg.Addr << "', timeout=" << (ctx->Deadline - TInstant::Now()).MilliSeconds() << "ms");

        with_lock(Mtx_) {
            ctx->GlobalRequestContextsListIterator = RequestContexts_.emplace(RequestContexts_.end(), ctx);
        }

        try {
            Client_->Request(NNeh::IMultiClient::TRequest(msg, ctx->Deadline, ctx.Get()));
        } catch (...) {
            with_lock(Mtx_) {
                RequestContexts_.erase(ctx->GlobalRequestContextsListIterator);
            }
            throw;
        }
    }

    void Run() {
        NNeh::IMultiClient::TEvent ev;
        while (Client_->Wait(ev)) {
            TRequestContextPtr ctx = ExtractRequestContext(reinterpret_cast<TRequestContext*>(ev.UserData));
            NNeh::TResponseRef response;
            if (ev.Type == NNeh::IMultiClient::TEvent::Response) {
                response = ev.Hndl->Get();
                static const TDuration minimalRequestDuration = TDuration::MilliSeconds(300);
                if (response && response->IsError() && ctx->AdditionalAttempts
                    && ((TInstant::Now() + minimalRequestDuration) < ctx->Deadline))
                {
                    bool retryableError = true;
                    if (response->GetErrorType() == NNeh::TError::ProtocolSpecific) {
                        auto httpCodeXX = response->GetErrorCode() / 100;
                        if (httpCodeXX == 4) {
                            retryableError = false;  // not retry all 4xx
                        } else if (httpCodeXX == 5 && response->GetErrorCode() != 503) {
                            retryableError = false;  // not retry 5xx exclude 503
                        }
                        if (retryableError) {
                            ctx->LogNextAttemptWarningCombo("http"sv, response->GetErrorCode(), response->GetErrorText());
                        }
                    } else if (const auto code = response->GetSystemErrorCode()) {
                        ctx->LogNextAttemptWarningCombo("sys"sv, response->GetSystemErrorCode(), response->GetErrorText());
                    } else {
                        ctx->LogNextAttemptWarningCombo("neh"sv, response->GetErrorCode(), response->GetErrorText());
                    }
                    if (retryableError) {
                        // try make additional attempt
                        ctx->AdditionalAttempts--;
                        ctx->AttemptNumber++;
                        if (!ctx->FirstError) {
                            ctx->FirstError = std::move(response);  // store first error for use instead timeout
                        }
                        try {
                            SendRequest(ctx->FirstError->Request, ctx);
                        } catch (...) {
                            ctx->LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << ctx->Id() << " use callback after next attempt fast fail: "sv << CurrentExceptionMessage());
                            try {
                                ctx->Callback(std::move(ctx->FirstError));
                            } catch (const std::exception& exc) {
                                ctx->LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << ctx->Id() << " THttpRequester: callback failed with exception: "sv << exc.what());
                            }
                        }
                        continue;
                    }
                }
            } else if (ev.Type == NNeh::IMultiClient::TEvent::Timeout) {
                ctx->LogContext.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << ctx->Id() << " MM request timeouted"sv);
                ev.Hndl->Cancel();
            } else {  // should be impossible
                Y_ASSERT(!"Unexpected event");
            }

            if (!ctx->Callback) {
                ctx->LogContext.LogEvent(
                    NEvClass::InfoMessage(
                        TStringBuilder()
                            << ctx->Id() << " response ("
                            << "duration=" << response->Duration << " "
                            << "firstLine=" << response->FirstLine << " "
                            << "dataLength=" << response->Data.size() << "): " << response->Data
                    )
                );
                continue;
            }

            try {
                ctx->LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << ctx->Id() << " use callback"sv);
                if (!response && ctx->FirstError) {
                    ctx->Callback(std::move(ctx->FirstError));  // use first catched error as final result (if not has response)
                } else {
                    ctx->Callback(std::move(response));
                }
            } catch (const std::exception& exc) {
                ctx->LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << ctx->Id() << " THttpRequester: callback failed with exception: "sv << exc.what());
            }
        }
    }

    TRequestContextPtr ExtractRequestContext(const TRequestContext* reqCtx) {
        with_lock(Mtx_) {
            auto iter = reqCtx->GlobalRequestContextsListIterator;
            TRequestContextPtr res(std::move(*iter));
            RequestContexts_.erase(iter);
            return res;
        }
    }

private:
    TMutex Mtx_;
    NNeh::TMultiClientPtr Client_;
    std::thread Thread_;
    TList<TRequestContextPtr> RequestContexts_;
};

}  // anonymous namespace


void SendHttpRequest(const NNeh::TMessage& msg, const TDuration timeout, unsigned additionalAttempts, THttpRequestCallback&& callback, TLogContext logContext, int subrequestId) {
    Y_STATIC_THREAD(THttpRequester) instance;
    instance.Get().SendRequest(msg, timeout, additionalAttempts, std::move(callback), std::move(logContext), subrequestId);
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
