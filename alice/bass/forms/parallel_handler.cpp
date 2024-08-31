#include "parallel_handler.h"

#include <alice/bass/util/error.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

TParallelHandlerExecutor::TParallelHandlerExecutor(IThreadPool& threadPool, const std::initializer_list<THandlerFactory>& handlers)
    : ThreadPool(threadPool)
    , Handlers(handlers)
{
}

TResultValue TParallelHandlerExecutor::Do(TRequestHandler& r) {
    if (Handlers.empty()) {
        return TError {
            TError::EType::SYSTEM,
            TStringBuilder{} << "No parallel handler for the form '" << r.Ctx().FormName() << "' found"
        };
    }
    for (auto& handler : Handlers) {
        if (auto error = AddFuture(r.Ctx(), handler)) {
            return error;
        }
    }
    for (auto& future : Futures) {
        try {
            TParallelHandlerResult result = future.GetValueSync();
            if (result.IsSuitable) {
                r.SwapContext(result.Context);
                return result.Result;
            }
        } catch (...) {
            LOG(ERR) << "Exception during handle TParallelHandler request for form '" << r.Ctx().FormName() << "'" << Endl;
        }
    }
    return TError {
        TError::EType::SYSTEM,
        TStringBuilder{} << "All parallel handlers fail for the form '" << r.Ctx().FormName() << "'"
    };
}

TResultValue TParallelHandlerExecutor::AddFuture(TContext& ctx, THandlerFactory handlerFactory) {
    auto promise = NThreading::NewPromise<TFuture::value_type>();
    auto cb = [promise, handlerFactory, &ctx]() mutable {
        try {
            THolder<IParallelHandler> handler = handlerFactory();
            if (!handler) {
                ythrow TErrorException(TError::EType::SYSTEM, TStringBuf("Unable to create parallel handler"));
            }

            ctx.UpdateLoggingReqInfo();
            // XXX not sure its ok
            TLogging::ReqInfo.Get().AppendToReqId(TStringBuilder() << "-parallel-" << handler->Name());
            TParallelHandlerResult result{.Context = ctx.Clone()};

            std::visit(result, handler->TryToHandle(*result.Context));

            promise.SetValue(std::move(result));
        } catch (...) {
            promise.SetException(CurrentExceptionMessage());
        }
    };
    if (ThreadPool.AddFunc(cb)) {
        Futures.push_back(promise.GetFuture());
    } else {
        return TError(TError::EType::SYSTEM, "Unable to run parallel handler (no threads)");
    }
    return ResultSuccess();
}

} // namespace NBASS
