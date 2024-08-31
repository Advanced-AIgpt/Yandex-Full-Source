#pragma once

#include <alice/bass/forms/vins.h>

#include <library/cpp/threading/future/future.h>

#include <util/thread/pool.h>

namespace NBASS {

struct TParallelHandlerResult;

// Interface for parallel form handler.
class IParallelHandler {
public:
    enum class ETryResult { NonSuitable, Success };
    using TTryResult = std::variant<ETryResult, TError>;

    virtual ~IParallelHandler() = default;

    virtual TTryResult TryToHandle(TContext& r) = 0;

    virtual TString Name() const = 0;
};

struct TParallelHandlerResult {
    TContext::TPtr Context;
    TResultValue Result;
    bool IsSuitable = false;

    void operator()(IParallelHandler::ETryResult resultType) {
        switch (resultType) {
            case IParallelHandler::ETryResult::Success:
                IsSuitable = true;
                break;
            case IParallelHandler::ETryResult::NonSuitable:
                IsSuitable = false;
                break;
        }
    }

    void operator()(TError&& e) {
        IsSuitable = true;
        Result = std::move(e);
    }
};

class TParallelHandlerExecutor : public IHandler {
public:
    using THandlerFactory = std::function<THolder<IParallelHandler>()>;

    TParallelHandlerExecutor(IThreadPool& threadPool, const std::initializer_list<THandlerFactory>& handlers);

    TResultValue Do(TRequestHandler& r) override;

private:
    using TFuture = NThreading::TFuture<TParallelHandlerResult>;
    using TFutureArray = TVector<TFuture>;
    using THandlersArray = TVector<THandlerFactory>;

    IThreadPool& ThreadPool;
    THandlersArray Handlers;
    TFutureArray Futures;

private:
    TResultValue AddFuture(TContext& ctx, THandlerFactory handlerFactory);
};

} // namespace NBASS
