package ru.yandex.alice.paskills.common.executor;

import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadPoolExecutor;

import ru.yandex.monlib.metrics.primitives.Rate;

class InstrumentedRejectedExecutionHandler implements RejectedExecutionHandler {

    private final RejectedExecutionHandler delegate;
    /**
     * Number of jobs pool rejected to submit a task
     */
    private final Rate rejected;

    InstrumentedRejectedExecutionHandler(RejectedExecutionHandler delegate, Rate rejected) {
        this.delegate = delegate;
        this.rejected = rejected;
    }

    @Override
    public void rejectedExecution(Runnable task, ThreadPoolExecutor executor) {
        rejected.inc();
        delegate.rejectedExecution(task, executor);
    }
}
