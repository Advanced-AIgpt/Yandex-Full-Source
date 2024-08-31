package ru.yandex.alice.paskill.dialogovo.utils.jetty;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.RejectedExecutionException;

import org.eclipse.jetty.util.thread.QueuedThreadPool;

import ru.yandex.alice.paskills.common.executor.InstrumentedRunnable;
import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.primitives.Rate;

public final class InstrumentedQueuedThreadPool extends QueuedThreadPool {

    /**
     * Number of jobs pool tries to submit a task
     */
    private final Rate submitted;
    /**
     * Number of jobs pool rejected to submit a task
     */
    private final Rate rejected;
    /**
     * Pool jobs waiting in queue time in milliseconds
     */
    private final Instrument waitings;
    /**
     * Pool job invocations
     */
    private final Instrument invocations;

    public InstrumentedQueuedThreadPool(
            int minThreads,
            int maxThreads,
            int idleTimeout,
            BlockingQueue<Runnable> queue,
            NamedSensorsRegistry sensorsRegistry) {
        super(maxThreads, minThreads, idleTimeout, queue);
        this.submitted = sensorsRegistry.rate("jobEnqueues");
        this.rejected = sensorsRegistry.rate("jobEnqueuesRejected");
        this.waitings = sensorsRegistry.instrument("waitings");
        this.invocations = sensorsRegistry.instrument("invocations");
    }

    @Override
    public void execute(Runnable job) {
        submitted.inc();
        try {
            super.execute(new InstrumentedRunnable(job, waitings, invocations));
        } catch (RejectedExecutionException e) {
            rejected.inc();
            throw e;
        }
    }
}
