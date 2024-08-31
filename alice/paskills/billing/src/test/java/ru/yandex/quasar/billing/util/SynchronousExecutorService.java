package ru.yandex.quasar.billing.util;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.AbstractExecutorService;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

public class SynchronousExecutorService extends AbstractExecutorService {

    private volatile boolean shutdown;

    public void shutdown() {
        shutdown = true;
    }

    @Nonnull
    public List<Runnable> shutdownNow() {
        // not implemented
        return Collections.emptyList();
    }

    public boolean isShutdown() {
        return shutdown;
    }

    public boolean isTerminated() {
        return shutdown;
    }

    public boolean awaitTermination(long time, @Nonnull TimeUnit unit) {
        // not implemented
        return true;
    }

    public void execute(@Nonnull Runnable runnable) {
        // run the task synchronously
        runnable.run();
    }

}
