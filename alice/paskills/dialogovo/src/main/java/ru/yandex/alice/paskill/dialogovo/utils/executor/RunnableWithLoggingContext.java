package ru.yandex.alice.paskill.dialogovo.utils.executor;

import java.util.Map;

import org.apache.logging.log4j.ThreadContext;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContext;
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskills.common.logging.protoseq.ThreadContextOverride;

class RunnableWithLoggingContext implements Runnable {

    private final Runnable delegate;
    private final LoggingContext loggingContext;
    private final RequestContext requestContext;
    private final RequestContext.Context context;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final DialogovoRequestContext.Context dialogovoContext;
    private final Map<String, String> callerThreadContext;

    RunnableWithLoggingContext(Runnable delegate, RequestContext requestContext,
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
    public void run() {
        LoggingContext externalLoggingContext = LoggingContextHolder.getCurrent();
        var externalContext = requestContext.getContext();
        var externalDialogovoContext = dialogovoRequestContext.getContext();

        LoggingContextHolder.setCurrent(loggingContext);
        requestContext.setContext(this.context);
        dialogovoRequestContext.setContext(dialogovoContext);
        try (var tco = new ThreadContextOverride(callerThreadContext)) {
            delegate.run();
        } finally {
            requestContext.setContext(externalContext);
            dialogovoRequestContext.setContext(externalDialogovoContext);
            LoggingContextHolder.setCurrent(externalLoggingContext);
        }
    }
}
