package ru.yandex.alice.paskills.my_alice.solomon;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class MethodRate {

    private final Rate ok;
    private final Rate error;

    public MethodRate(MetricRegistry metricRegistry, String component, String method) {
        this.ok = metricRegistry.rate(
                "ok", Labels.of("component", component, "method", method));
        this.error = metricRegistry.rate(
                "error", Labels.of("component", component, "method", method));
    }

    public void ok() {
        this.ok.inc();
    }

    public void error() {
        this.error.inc();
    }
}
