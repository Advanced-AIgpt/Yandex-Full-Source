package ru.yandex.alice.paskills.common.executor;

import java.time.Duration;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.util.concurrent.ThreadFactoryBuilder;

import ru.yandex.alice.paskills.common.solomon.utils.Instrument;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.primitives.Rate;

public class InstrumentedExecutorService implements ExecutorService {

    public static final int TIMEOUT_DELTA = 100;

    private final ExecutorService delegate;

    /**
     * Number of jobs pool tries to submit a task
     */
    private final Rate submitted;
    /**
     * Pool jobs waiting in queue time in milliseconds
     */
    private final Instrument waitings;
    /**
     * Pool job invocations
     */
    private final Instrument invocations;
    /**
     * Pool job timeouts
     */
    private final Rate timeouts;
    private final String name;

    public InstrumentedExecutorService(
            NamedSensorsRegistry sensorsRegistry,
            PoolConfig poolConfig,
            BlockingQueue<Runnable> queue,
            ThreadFactory threadFactory,
            RejectedExecutionHandler handler,
            String name) {
        this.name = name;
        this.submitted = sensorsRegistry.rate("submitted");
        this.waitings = sensorsRegistry.instrument("waitings");
        this.invocations = sensorsRegistry.instrument("invocations");
        this.timeouts = sensorsRegistry.rate("timeouts");
        var threadPoolExecutor = new ThreadPoolExecutor(
                poolConfig.getCorePoolSize(),
                poolConfig.getMaximumPoolSize(),
                poolConfig.getKeepAliveTime(),
                poolConfig.getUnit(),
                queue,
                new ThreadFactoryBuilder()
                        .setThreadFactory(threadFactory)
                        .setNameFormat(name + "-%d")
                        .build(),
                new InstrumentedRejectedExecutionHandler(handler,
                        sensorsRegistry.rate("rejected")));
        this.delegate = threadPoolExecutor;

        sensorsRegistry.lazyGauge("queueSize", () -> threadPoolExecutor.getQueue().size());
        sensorsRegistry.lazyGauge("queueRemainingCapacity", () -> threadPoolExecutor.getQueue().remainingCapacity());
        sensorsRegistry.lazyGauge("poolSize", threadPoolExecutor::getPoolSize);
        sensorsRegistry.lazyGauge("activeCount", threadPoolExecutor::getActiveCount);
        sensorsRegistry.lazyGauge("taskCount", threadPoolExecutor::getTaskCount);
        sensorsRegistry.lazyGauge("completedTaskCount", threadPoolExecutor::getCompletedTaskCount);
        sensorsRegistry.lazyGauge("corePoolSize", threadPoolExecutor::getCorePoolSize);
        sensorsRegistry.lazyGauge("largestPoolSize", threadPoolExecutor::getLargestPoolSize);
        sensorsRegistry.lazyGauge("maximumPoolSize", threadPoolExecutor::getMaximumPoolSize);
    }

    public InstrumentedExecutorService(
            NamedSensorsRegistry sensorsRegistry,
            String name,
            ExecutorService delegate) {
        this.name = name;
        this.submitted = sensorsRegistry.rate("submitted");
        this.waitings = sensorsRegistry.instrument("waitings");
        this.invocations = sensorsRegistry.instrument("invocations");
        this.timeouts = sensorsRegistry.rate("timeouts");
        this.delegate = delegate;
    }

    @Override
    public void shutdown() {
        delegate.shutdown();
    }

    @Override
    public List<Runnable> shutdownNow() {
        return delegate.shutdownNow();
    }

    @Override
    public boolean isShutdown() {
        return delegate.isShutdown();
    }

    @Override
    public boolean isTerminated() {
        return delegate.isTerminated();
    }

    @Override
    public boolean awaitTermination(long timeout, @Nonnull TimeUnit unit) throws InterruptedException {
        return delegate.awaitTermination(timeout, unit);
    }

    protected <T> Callable<T> withContext(@Nonnull Callable<T> task) {
        return task;
    }

    protected Runnable withContext(Runnable task) {
        return task;
    }

    protected <T> Supplier<T> withContext(@Nonnull Supplier<T> supplier) {
        return supplier;
    }

    @Override
    public <T> Future<T> submit(@Nonnull Callable<T> task) {
        submitted.inc();
        return delegate.submit(withContext(task));
    }

    @Override
    public <T> Future<T> submit(@Nonnull Runnable task, T result) {
        submitted.inc();
        return delegate.submit(withContext(task), result);
    }

    @Override
    public Future<?> submit(@Nonnull Runnable task) {
        submitted.inc();
        return delegate.submit(withContext(task));
    }

    @Override
    public <T> List<Future<T>> invokeAll(Collection<? extends Callable<T>> tasks) throws InterruptedException {
        submitted.add(tasks.size());
        return delegate.invokeAll(tasks.stream().map(this::withContext).collect(Collectors.toList()));
    }

    @Override
    public <T> List<Future<T>> invokeAll(Collection<? extends Callable<T>> tasks, long timeout, @Nonnull TimeUnit unit)
            throws InterruptedException {
        submitted.add(tasks.size());
        return delegate.invokeAll(tasks.stream().map(this::withContext).collect(Collectors.toList()), timeout, unit);
    }

    @Override
    public <T> T invokeAny(Collection<? extends Callable<T>> tasks) throws InterruptedException, ExecutionException {
        submitted.add(tasks.size());
        return delegate.invokeAny(tasks.stream().map(this::withContext).collect(Collectors.toList()));
    }

    @Override
    public <T> T invokeAny(Collection<? extends Callable<T>> tasks, long timeout, @Nonnull TimeUnit unit)
            throws InterruptedException, ExecutionException, TimeoutException {
        submitted.add(tasks.size());
        return delegate.invokeAny(tasks.stream().map(this::withContext).collect(Collectors.toList()), timeout, unit);
    }

    @Override
    public void execute(@Nonnull Runnable command) {
        submitted.inc();
        delegate.execute(withContext(command));
    }

    private <T> Supplier<T> instrumented(@Nonnull Supplier<T> supplier) {
        return new InstrumentedSupplier<>(withContext(supplier), waitings, invocations);
    }

    private Runnable instrumented(@Nonnull Runnable runnable) {
        return new InstrumentedRunnable(withContext(runnable), waitings, invocations);
    }

    public <T> CompletableFuture<T> supplyAsyncInstrumentedWithoutTimeout(@Nonnull Supplier<T> supplier) {
        return CompletableFuture.supplyAsync(instrumented(supplier), this);
    }

    public <T> CompletableFuture<T> supplyAsyncInstrumented(@Nonnull Supplier<T> supplier,
                                                            Duration timeout) {
        return CompletableFuture.supplyAsync(instrumented(supplier), this)
                .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
                .handle((result, throwable) -> {
                    if (throwable != null) {
                        if (throwable instanceof TimeoutException) {
                            timeouts.inc();
                        }

                        throw new CompletionException("CompletionException on service: " + name, throwable);
                    }

                    return result;
                });
    }

    public <T> CompletableFuture<T> supplyAsyncInstrumented(@Nonnull Supplier<T> supplier,
                                                            Duration timeout,
                                                            Supplier<T> defaultValueOnTimeoutSupplier) {
        return CompletableFuture.supplyAsync(instrumented(supplier), this)
                .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
                .handle((result, throwable) -> {
                    if (throwable != null) {
                        if (throwable instanceof TimeoutException) {
                            timeouts.inc();
                            return defaultValueOnTimeoutSupplier.get();
                        } else {
                            throw new CompletionException("CompletionException on service: " + name, throwable);
                        }
                    }

                    return result;
                });
    }

    public CompletableFuture<Void> runAsyncInstrumentedWithoutTimeout(@Nonnull Runnable runnable) {
        return CompletableFuture.runAsync(instrumented(runnable), this);
    }

    public CompletableFuture<Void> runAsyncInstrumented(@Nonnull Runnable runnable, Duration timeout) {
        return CompletableFuture.runAsync(instrumented(runnable), this)
                .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
                .whenComplete((result, error) -> {
                    if (error != null) {
                        if (error instanceof TimeoutException) {
                            timeouts.inc();
                        }

                        throw new CompletionException("CompletionException on service: " + name, error);
                    }
                });
    }

    @Override
    public String toString() {
        return delegate.toString();
    }

    public void gracefulShutdown(int timeout, TimeUnit timeUnit) {
        shutdown(); // Disable new tasks from being submitted
        try {
            // Wait a while for existing tasks to terminate
            if (!awaitTermination(timeout, timeUnit)) {
                shutdownNow(); // Cancel currently executing tasks
                // Wait a while for tasks to respond to being cancelled
                if (!awaitTermination(timeout, timeUnit)) {
                    System.err.println("Pool " + this.name + " did not terminate");
                }
            }
        } catch (InterruptedException ie) {
            // (Re-)Cancel if current thread also interrupted
            shutdownNow();
            // Preserve interrupt status
            Thread.currentThread().interrupt();
        }
    }

    public void gracefulShutdown() {
        gracefulShutdown(60, TimeUnit.SECONDS);
    }
}
