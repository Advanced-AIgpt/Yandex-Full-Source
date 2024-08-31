package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.util.Map;
import java.util.function.Supplier;

import com.google.common.base.Stopwatch;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ProxyType;
import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public class SolomonHelper {

    private static final Supplier<HistogramCollector> EXTERNAL_SENSOR_COLLECTOR_SUPPLIER = () -> Histograms
            .explicit(100, 200, 300, 500, 1000, 1300, 1500, 2000, 2500,
                    2600, 2800, 2900, 3000, 3100, 3200, 3300, 3500);
    private static final Supplier<HistogramCollector> INTERNAL_SENSOR_COLLECTOR_SUPPLIER =
            () -> Histograms.linear(45, 0, 100);
    private static final int LONG_REQUEST_COUNTER_THRESHOLD_MS = 2500;

    private final SkillInfo skill;
    private final MetricRegistry internalMetricRegistry;
    private final MetricRegistry externalMetricRegistry;
    private final SourceType sourceType;
    private final ProxyType proxy;
    private Labels externalSolomonLabels;
    private Labels internalSolomonLabels;
    private final Stopwatch stopwatch = Stopwatch.createStarted();

    SolomonHelper(SkillInfo skill,
                  MetricRegistry internalMetricRegistry,
                  MetricRegistry externalMetricRegistry,
                  SourceType sourceType,
                  ProxyType proxy) {

        this.skill = skill;
        this.sourceType = sourceType;
        this.internalMetricRegistry = internalMetricRegistry;
        this.externalMetricRegistry = externalMetricRegistry;
        this.externalSolomonLabels = getExternalSolomonLabels(skill, sourceType);
        this.internalSolomonLabels = getInternalSolomonLabels(sourceType, proxy);
        this.proxy = proxy;
    }

    public void markCommandStatusExternal(String status) {
        externalSolomonLabels = externalSolomonLabels.add("command_status", status);
    }

    public void markCommandStatusInternal(String status) {
        internalSolomonLabels = internalSolomonLabels.add("command_status", status);
    }

    public Stopwatch getStopwatch() {
        return stopwatch;
    }

    public static Labels getExternalSolomonLabels(SkillInfo skill, SourceType sourceType) {
        return Labels.of(Map.of(
                "skill_id", skill.getId(),
                "skill_type", skill.getChannel().value(),
                "command", "webhookRequest", // TODO:
                "source", sourceType.getCode(),
                "api_version", "1.0", // TODO: add api_version from request,
                "monitoring_type", skill.getMonitoringType() != null
                        ? skill.getMonitoringType().getCode() : SkillInfo.MonitoringType.NONMONITORED.getCode()
        ));
    }

    public static Labels getInternalSolomonLabels(SourceType sourceType, ProxyType proxy) {
        return Labels.of(
                "command", "webhookRequest",
                "source", sourceType.getCode(),
                "proxy", proxy.value()
        );
    }

    public void stop() {
        stopwatch.stop();
    }

    public void complete() {

        var elapsedMillis = stopwatch.elapsed().toMillis();

        if (sourceType == SourceType.USER || skill.isMonitored()) {
            externalMetricRegistry
                    .histogramRate("request_duration", externalSolomonLabels,
                            EXTERNAL_SENSOR_COLLECTOR_SUPPLIER)
                    .record(elapsedMillis);

            externalMetricRegistry.rate("request", externalSolomonLabels).inc();

            if (elapsedMillis > LONG_REQUEST_COUNTER_THRESHOLD_MS) {
                externalMetricRegistry.rate("long_request", externalSolomonLabels).inc();
            }
        }

        internalMetricRegistry
                .histogramRate("request_duration", internalSolomonLabels, INTERNAL_SENSOR_COLLECTOR_SUPPLIER)
                .record(elapsedMillis);

        internalMetricRegistry.rate("request", internalSolomonLabels).inc();
    }

}
