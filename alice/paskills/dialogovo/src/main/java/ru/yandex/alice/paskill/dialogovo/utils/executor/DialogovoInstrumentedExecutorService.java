package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.executor.InstrumentedExecutorService;
import ru.yandex.alice.paskills.common.executor.PoolConfig;
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry;

public final class DialogovoInstrumentedExecutorService extends InstrumentedExecutorService {

    public static final int TIMEOUT_DELTA = 100;

    private final RequestContext requestContext;
    private final DialogovoRequestContext dialogovoRequestContext;

    @SuppressWarnings("ParameterNumber")
    public DialogovoInstrumentedExecutorService(
            NamedSensorsRegistry sensorsRegistry,
            RequestContext requestContext,
            DialogovoRequestContext dialogovoRequestContext,
            PoolConfig poolConfig,
            BlockingQueue<Runnable> queue,
            ThreadFactory threadFactory,
            RejectedExecutionHandler handler,
            String name) {
        super(sensorsRegistry, poolConfig, queue, threadFactory, handler, name);
        this.requestContext = requestContext;
        this.dialogovoRequestContext = dialogovoRequestContext;
    }

    DialogovoInstrumentedExecutorService(
            NamedSensorsRegistry sensorsRegistry,
            RequestContext requestContext,
            DialogovoRequestContext dialogovoRequestContext,
            String name,
            ExecutorService delegate) {
        super(sensorsRegistry, name, delegate);
        this.requestContext = requestContext;
        this.dialogovoRequestContext = dialogovoRequestContext;
    }

    @Override
    protected <T> Callable<T> withContext(@Nonnull Callable<T> task) {
        return new CallableWithLoggingContext<>(task, requestContext, dialogovoRequestContext);
    }

    @Override
    protected Runnable withContext(Runnable task) {
        return new RunnableWithLoggingContext(task, requestContext, dialogovoRequestContext);
    }

    @Override
    protected <T> Supplier<T> withContext(@Nonnull Supplier<T> supplier) {
        return new SupplierWithLoggingContext<>(supplier, requestContext, dialogovoRequestContext);
    }

}
