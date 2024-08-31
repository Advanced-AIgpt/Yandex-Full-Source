package ru.yandex.alice.paskills.common.solomon.utils;

import java.util.function.Supplier;

import javax.annotation.Nullable;

import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public final class RequestSensors {
    private final Rate requestRate;
    private final Rate failureRate;
    private final Histogram requestTimings;

    private RequestSensors(Rate requestRate, Rate failureRate, Histogram requestTimings) {
        this.requestRate = requestRate;
        this.failureRate = failureRate;
        this.requestTimings = requestTimings;
    }

    public static RequestSensors withLabels(MetricRegistry registry, Labels labels) {
        return withLabels(registry, labels, null);
    }

    public static RequestSensors withLabels(MetricRegistry registry, Labels labels, @Nullable String prefix) {
        return withLabels(registry, labels, prefix, () -> Histograms.exponential(19, 1.5d, 10.3d / 1.5d));
    }

    public static RequestSensors withLabels(MetricRegistry registry, Labels labels, @Nullable String prefix,
                                            Supplier<HistogramCollector> histogramCollectorSupplier) {

        var p = prefix != null ? prefix : "";

        return new RequestSensors(
                registry.rate(p + "requests_rate", labels),
                registry.rate(p + "requests_failure_rate", labels),
                // scale to have a bucket on 3 seconds
                registry.histogramRate(p + "requests_duration", labels, histogramCollectorSupplier)
        );
    }

    public <T> T measure(Supplier<T> supplier) {
        requestRate.inc();
        long start = System.nanoTime();
        try {
            return supplier.get();
        } catch (RuntimeException e) {
            failureRate.inc();
            throw e;
        } finally {
            requestTimings.record((System.nanoTime() - start) / 1_000_000);
        }
    }

    public Rate getRequestRate() {
        return requestRate;
    }

    public Rate getFailureRate() {
        return failureRate;
    }

    public Histogram getRequestTimings() {
        return requestTimings;
    }
}
