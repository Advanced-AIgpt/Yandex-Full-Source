package ru.yandex.alice.paskills.common.solomon.utils;

import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;

/**
 * Accumulates common metrics computations
 */
public class Instrument {

    private final Histogram times;
    private final Rate errors;
    private final Rate invocations;

    public Instrument(NamedSensorsRegistry sensorsRegistry) {
        this.times = sensorsRegistry.histogram("times");
        this.errors = sensorsRegistry.rate("errors");
        this.invocations = sensorsRegistry.rate("invocations");
    }

    public Instrument(NamedSensorsRegistry sensorsRegistry, Supplier<HistogramCollector> supplier) {
        this.times = sensorsRegistry.histogram("times", supplier);
        this.errors = sensorsRegistry.rate("errors");
        this.invocations = sensorsRegistry.rate("invocations");
    }

    public void measure(long duration, boolean success) {
        invocations.inc();
        times.record(duration);
        if (!success) {
            errors.inc();
        }
    }

    public <T> T time(Supplier<T> supplier) {
        long start = System.nanoTime();
        Throwable ex = null;
        try {
            return supplier.get();
        } catch (Exception e) {
            ex = e;
            throw e;
        } finally {
            long duration = (System.nanoTime() - start) / 1_000_000;
            measure(duration, ex == null);
        }
    }

    public void time(Runnable runnable) {
        long start = System.nanoTime();
        Throwable ex = null;
        try {
            runnable.run();
        } catch (Exception e) {
            ex = e;
            throw e;
        } finally {
            long duration = (System.nanoTime() - start) / 1_000_000;
            measure(duration, ex == null);
        }
    }

    public <T> CompletableFuture<T> timeFuture(Supplier<CompletableFuture<T>> futureSupplier) {
        long start = System.nanoTime();
        return futureSupplier.get().whenComplete((res, ex) ->
                measure((System.nanoTime() - start) / 1_000_000, ex == null)
        );
    }
}
