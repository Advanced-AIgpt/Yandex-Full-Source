#pragma once

#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/core/future.h>
#include <util/generic/function.h>
#include <util/generic/queue.h>
#include <util/thread/pool.h>

namespace NGranet {

template<class ProcessFuncType, class PostprocessFuncType>
class TParallelProcessorWithOrderedPostprocess {
public:
    using OutputType = TFunctionResult<ProcessFuncType>;

    TParallelProcessorWithOrderedPostprocess() = delete;
    TParallelProcessorWithOrderedPostprocess(ProcessFuncType processFunc, PostprocessFuncType postprocessFunc,
            IThreadPool* threadPool, size_t resultQueueLimit)
        : ProcessFunc(std::move(processFunc))
        , PostprocessFunc(std::move(postprocessFunc))
        , ThreadPool(threadPool)
        , ResultQueueLimit(resultQueueLimit)
    {
        Y_ENSURE(threadPool);
    }

    template<class InputType>
    void Push(InputType input) {
        Queue.emplace(NThreading::Async(std::function<OutputType()>(std::bind(ProcessFunc, std::move(input))), *ThreadPool));
        while (!Queue.empty() && (Queue.size() >= ResultQueueLimit || Queue.front().HasValue())) {
            PostprocessFunc(Queue.front().ExtractValueSync());
            Queue.pop();
        }
    }

    void Finalize() {
        while (!Queue.empty()) {
            PostprocessFunc(Queue.front().ExtractValueSync());
            Queue.pop();
        }
    }

private:
    ProcessFuncType ProcessFunc;
    PostprocessFuncType PostprocessFunc;
    IThreadPool* ThreadPool = nullptr;
    size_t ResultQueueLimit = 0;
    TQueue<NThreading::TFuture<NThreading::TFutureType<OutputType>>> Queue;
};

} // namespace NGranet
