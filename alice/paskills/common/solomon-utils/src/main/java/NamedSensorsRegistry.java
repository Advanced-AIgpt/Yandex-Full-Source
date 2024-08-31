package ru.yandex.alice.paskills.common.solomon.utils;

import java.util.function.DoubleSupplier;
import java.util.function.LongSupplier;
import java.util.function.Supplier;

import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.LazyCounter;
import ru.yandex.monlib.metrics.primitives.LazyGaugeDouble;
import ru.yandex.monlib.metrics.primitives.LazyGaugeInt64;
import ru.yandex.monlib.metrics.primitives.LazyRate;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class NamedSensorsRegistry {

    private final MetricRegistry delegate;
    private final String name;

    public NamedSensorsRegistry(MetricRegistry delegate, String name) {
        this.delegate = delegate;
        this.name = name;
    }

    public NamedSensorsRegistry sub(String subName) {
        return new NamedSensorsRegistry(delegate, name + "." + subName);
    }

    public NamedSensorsRegistry withLabels(Labels labels) {
        return new NamedSensorsRegistry(delegate.subRegistry(labels), name);
    }

    public Instrument instrument(String subName) {
        return new Instrument(sub(subName));
    }

    public Instrument instrument(String subName, Supplier<HistogramCollector> supplier) {
        return new Instrument(sub(subName), supplier);
    }

    public Rate rate(String subName) {
        return delegate.rate(name + "." + subName);
    }

    public LazyRate lazyRate(String subName, LongSupplier supplier) {
        return delegate.lazyRate(name + "." + subName, supplier);
    }

    public LazyGaugeInt64 lazyGauge(String subName, LongSupplier supplier) {
        return delegate.lazyGaugeInt64(name + "." + subName, supplier);
    }

    public LazyGaugeDouble lazyGauge(String subName, DoubleSupplier supplier) {
        return delegate.lazyGaugeDouble(name + "." + subName, supplier);
    }

    public GaugeInt64 gauge(String subName) {
        return delegate.gaugeInt64(name + "." + subName);
    }

    public Histogram histogram(String subName) {
        return delegate.histogramRate(name + "." + subName, () -> Histograms.exponential(50, 1.5d, 10.3d / 1.5d));
    }

    public Histogram histogram(String subName, Supplier<HistogramCollector> supplier) {
        return delegate.histogramRate(name + "." + subName, supplier);
    }

    public LazyCounter lazyCounter(String subName, LongSupplier supplier) {
        return delegate.lazyCounter(name + "." + subName, supplier);
    }
}
