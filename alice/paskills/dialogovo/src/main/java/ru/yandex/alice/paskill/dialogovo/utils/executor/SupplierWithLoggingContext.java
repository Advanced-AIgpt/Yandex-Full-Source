package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.Map;
import java.util.function.Supplier;

import org.apache.logging.log4j.ThreadContext;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.logging.protoseq.ThreadContextOverride;

class SupplierWithLoggingContext<T> implements Supplier<T> {

    private final Supplier<T> delegate;
    private final LoggingContext loggingContext;
    private final RequestContext requestContext;
    private final RequestContext.Context context;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final DialogovoRequestContext.Context dialogovoContext;
    private final Map<String, String> callerThreadContext;

    SupplierWithLoggingContext(Supplier<T> delegate, RequestContext requestContext,
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
    public T get() {
        LoggingContext externalLoggingContext = LoggingContextHolder.getCurrent();
        var externalContext = requestContext.getContext();
        var externalDialogovoContext = dialogovoRequestContext.getContext();

        LoggingContextHolder.setCurrent(loggingContext);
        requestContext.setContext(context);
        dialogovoRequestContext.setContext(dialogovoContext);
        try (var tco = new ThreadContextOverride(callerThreadContext)) {
            return delegate.get();
        } finally {
            requestContext.setContext(externalContext);
            dialogovoRequestContext.setContext(externalDialogovoContext);
            LoggingContextHolder.setCurrent(externalLoggingContext);
        }
    }
}
