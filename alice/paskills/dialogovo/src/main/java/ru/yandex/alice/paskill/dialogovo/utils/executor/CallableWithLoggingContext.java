package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.Map;
import java.util.concurrent.Callable;

import javax.annotation.Nullable;

import org.apache.logging.log4j.ThreadContext;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.logging.protoseq.ThreadContextOverride;

class CallableWithLoggingContext<V> implements Callable<V> {

    private final Callable<V> delegate;
    private final LoggingContext loggingContext;
    private final RequestContext requestContext;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final DialogovoRequestContext.Context dialogovoContext;
    private final RequestContext.Context context;
    private final Map<String, String> callerThreadContext;

    CallableWithLoggingContext(Callable<V> delegate, RequestContext requestContext,
                               DialogovoRequestContext dialogovoRequestContext) {
        this.delegate = delegate;
        this.requestContext = requestContext;
        this.context = requestContext.getContext().makeCopy();
        this.loggingContext = LoggingContextHolder.getCurrent();
        this.callerThreadContext = ThreadContext.getContext();
        this.dialogovoRequestContext = dialogovoRequestContext;
        this.dialogovoContext = dialogovoRequestContext.getContext().makeCopy();
    }

    @Override
    @Nullable
    public V call() throws Exception {
        LoggingContext externalLoggingContext = LoggingContextHolder.getCurrent();
        var externalContext = requestContext.getContext();
        var externalDialogovoContext = dialogovoRequestContext.getContext();
        LoggingContextHolder.setCurrent(loggingContext);
        requestContext.setContext(context);
        dialogovoRequestContext.setContext(dialogovoContext);
        try (var tco = new ThreadContextOverride(callerThreadContext)) {
            return delegate.call();
        } finally {
            requestContext.setContext(externalContext);
            dialogovoRequestContext.setContext(externalDialogovoContext);
            LoggingContextHolder.setCurrent(externalLoggingContext);
        }
    }
}
