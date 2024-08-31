package ru.yandex.alice.paskill.dialogovo.utils.executor;

import com.google.common.util.concurrent.MoreExecutors;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class TestExecutorsFactory {
    private TestExecutorsFactory() {
        throw new UnsupportedOperationException();
    }

    public static DialogovoInstrumentedExecutorService newSingleThreadExecutor() {
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), new RequestContext(),
                new DialogovoRequestContext());

        return executorsFactory.newSingleThreadExecutor("test");
    }

    public static DialogovoInstrumentedExecutorService newSingleThreadExecutor(RequestContext requestContext) {
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), requestContext,
                new DialogovoRequestContext());

        return executorsFactory.newSingleThreadExecutor("test");
    }

    public static DialogovoInstrumentedExecutorService newFixedThreadPool(int threads) {
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), new RequestContext(),
                new DialogovoRequestContext());

        return executorsFactory.fixedThreadPool(threads, "test");
    }

    // everything is executed in the thread calling "submit"
    public static DialogovoInstrumentedExecutorService syncExecutor(RequestContext requestContext,
                                                                    DialogovoRequestContext dialogovoRequestContext) {
        var metricRegistry = new MetricRegistry();
        var namedSensorsRegistry = new NamedSensorsRegistry(metricRegistry.subRegistry("pool", "test"),
                "thread");

        return new DialogovoInstrumentedExecutorService(
                namedSensorsRegistry,
                requestContext,
                dialogovoRequestContext,
                "test",
                MoreExecutors.newDirectExecutorService());
    }

    public static DialogovoInstrumentedExecutorService newFixedThreadPool(int threads, int queueCapacity) {
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), new RequestContext(),
                new DialogovoRequestContext());

        return executorsFactory.fixedThreadPool(threads, queueCapacity, "test");
    }

    public static DialogovoInstrumentedExecutorService newFixedThreadPool(int threads, RequestContext requestContext) {
        ExecutorsFactory executorsFactory = new ExecutorsFactory(new MetricRegistry(), requestContext,
                new DialogovoRequestContext());

        return executorsFactory.fixedThreadPool(threads, "test");
    }
}
