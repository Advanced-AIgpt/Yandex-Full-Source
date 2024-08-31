package ru.yandex.alice.paskill.dialogovo.webhook.client;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ProxyType;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Component
class SolomonHelperFactory {

    private final MetricRegistry internalMetricRegistry;
    private final MetricRegistry externalMetricRegistry;

    SolomonHelperFactory(
            @Qualifier("internalMetricRegistry") MetricRegistry internalMetricRegistry,
            @Qualifier("externalMetricRegistry") MetricRegistry externalMetricRegistry
    ) {

        this.internalMetricRegistry = internalMetricRegistry;
        this.externalMetricRegistry = externalMetricRegistry;
    }

    public SolomonHelper start(SkillInfo skill, SourceType sourceType, ProxyType proxy) {
        return new SolomonHelper(skill, internalMetricRegistry, externalMetricRegistry, sourceType, proxy);
    }
}
