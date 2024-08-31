package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.executor.PoolConfig;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Component
public class ExecutorsFactory {
    private static final Logger logger = LogManager.getLogger();
    private static final RejectedExecutionHandler DEFAULT_HANDLER = new ThreadPoolExecutor.AbortPolicy();
    private final MetricRegistry metricRegistry;
    private final Set<ExecutorService> registeredExecutors = new HashSet<>();
    private final RequestContext requestContext;
    private final DialogovoRequestContext dialogovoRequestContext;

    public ExecutorsFactory(
            MetricRegistry metricRegistry,
            RequestContext requestContext,
            DialogovoRequestContext dialogovoRequestContext
    ) {
        this.metricRegistry = metricRegistry;
        this.requestContext = requestContext;
        this.dialogovoRequestContext = dialogovoRequestContext;
    }

    public DialogovoInstrumentedExecutorService fixedThreadPool(int threads, String name) {
        return fixedThreadPool(threads, Executors.defaultThreadFactory(), name);
    }

    public DialogovoInstrumentedExecutorService fixedThreadPool(int threads, int queueCapacity, String name) {
        return fixedThreadPool(threads, queueCapacity, Executors.defaultThreadFactory(), name);
    }

    public DialogovoInstrumentedExecutorService cachedBoundedThreadPool(int corePoolSize, int maximumPoolSize,
                                                                        int queueCapacity, String name) {
        NamedSensorsRegistry namedMetricRegistry = getMetricRegistry(name);

        DialogovoInstrumentedExecutorService executorService = new DialogovoInstrumentedExecutorService(
                namedMetricRegistry,
                requestContext,
                dialogovoRequestContext,
                new PoolConfig(
                        corePoolSize,
                        maximumPoolSize,
                        60L,
                        TimeUnit.SECONDS),
                new LinkedBlockingQueue<>(queueCapacity),
                Executors.defaultThreadFactory(),
                DEFAULT_HANDLER,
                name);

        register(executorService);
        return executorService;
    }

    public DialogovoInstrumentedExecutorService newSingleThreadExecutor(String name) {
        NamedSensorsRegistry namedMetricRegistry = getMetricRegistry(name);

        DialogovoInstrumentedExecutorService executorService = new DialogovoInstrumentedExecutorService(
                namedMetricRegistry,
                requestContext,
                dialogovoRequestContext,
                new PoolConfig(1,
                        1,
                        60L,
                        TimeUnit.SECONDS),
                new LinkedBlockingQueue<>(1),
                Executors.defaultThreadFactory(),
                DEFAULT_HANDLER,
                name);

        register(executorService);
        return executorService;
    }

    private DialogovoInstrumentedExecutorService fixedThreadPool(
            int threads,
            int queueCapacity,
            ThreadFactory threadFactory,
            String name
    ) {
        NamedSensorsRegistry namedMetricRegistry = getMetricRegistry(name);

        DialogovoInstrumentedExecutorService executorService = new DialogovoInstrumentedExecutorService(
                namedMetricRegistry,
                requestContext,
                dialogovoRequestContext,
                new PoolConfig(threads,
                        threads,
                        0L,
                        TimeUnit.MILLISECONDS),
                new LinkedBlockingQueue<>(queueCapacity),
                threadFactory,
                DEFAULT_HANDLER,
                name);

        register(executorService);
        return executorService;
    }

    private DialogovoInstrumentedExecutorService fixedThreadPool(
            int threads,
            ThreadFactory threadFactory,
            String name
    ) {
        NamedSensorsRegistry namedMetricRegistry = getMetricRegistry(name);

        DialogovoInstrumentedExecutorService executorService = new DialogovoInstrumentedExecutorService(
                namedMetricRegistry,
                requestContext,
                dialogovoRequestContext,
                new PoolConfig(threads,
                        threads,
                        0L,
                        TimeUnit.MILLISECONDS),
                new LinkedBlockingQueue<>(),
                threadFactory,
                DEFAULT_HANDLER,
                name);

        register(executorService);
        return executorService;
    }

    private NamedSensorsRegistry getMetricRegistry(String name) {
        return new NamedSensorsRegistry(this.metricRegistry.subRegistry("pool", name), "thread");
    }

    public void register(ExecutorService executorService) {
        registeredExecutors.add(executorService);
    }

    public void shutdown() {
        registeredExecutors.forEach(executor -> ExecutorUtils.shutdownExecutor(executor, 1, TimeUnit.SECONDS));
        logger.info("Executor shutdown service finished");
    }
}
