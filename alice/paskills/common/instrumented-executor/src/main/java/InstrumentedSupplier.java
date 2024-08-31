package ru.yandex.alice.paskills.common.executor;

import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;

import ru.yandex.alice.paskills.common.solomon.utils.Instrument;

class InstrumentedSupplier<T> implements Supplier<T> {

    private final Supplier<T> delegate;
    /**
     * Pool jobs waiting in queue time in milliseconds
     */
    private final Instrument waitings;
    /**
     * Pool job invocations
     */
    private final Instrument invocations;
    private final Stopwatch timer;

    InstrumentedSupplier(Supplier<T> delegate, Instrument waitings, Instrument invocations) {
        this.delegate = delegate;
        this.waitings = waitings;
        this.invocations = invocations;
        this.timer = Stopwatch.createStarted();
    }

    @Override
    @Nullable
    public T get() {
        timer.stop();
        waitings.measure(timer.elapsed(TimeUnit.MILLISECONDS), true);

        boolean success = false;
        timer.start();
        try {
            T value = delegate.get();
            success = true;
            return value;
        } finally {
            invocations.measure(timer.elapsed(TimeUnit.MILLISECONDS), success);
        }
    }
}
