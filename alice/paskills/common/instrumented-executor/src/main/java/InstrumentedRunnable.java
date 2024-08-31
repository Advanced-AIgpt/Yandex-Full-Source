package ru.yandex.alice.paskills.common.executor;

import java.util.concurrent.TimeUnit;

import com.google.common.base.Stopwatch;

import ru.yandex.alice.paskills.common.solomon.utils.Instrument;

public class InstrumentedRunnable implements Runnable {

    private final Runnable delegate;
    /**
     * Pool jobs waiting in queue time in milliseconds
     */
    private final Instrument waitings;
    /**
     * Pool job invocations
     */
    private final Instrument invocations;
    private final Stopwatch timer;

    public InstrumentedRunnable(Runnable delegate, Instrument waitings, Instrument invocations) {
        this.delegate = delegate;
        this.waitings = waitings;
        this.invocations = invocations;
        this.timer = Stopwatch.createStarted();
    }

    @Override
    public void run() {
        timer.stop();
        waitings.measure(timer.elapsed(TimeUnit.MILLISECONDS), true);

        boolean success = false;
        timer.start();
        try {
            delegate.run();
            success = true;
        } finally {
            invocations.measure(timer.elapsed(TimeUnit.MILLISECONDS), success);
        }
    }
}
