package ru.yandex.alice.paskill.dialogovo.test;

import java.time.Instant;
import java.util.Collections;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.web.server.LocalServerPort;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.kronstadt.test.ClientInfoTestUtils;
import ru.yandex.alice.megamind.protos.scenarios.RequestProto;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.domain.Session;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.domain.SkillProcessRequest;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessor;
import ru.yandex.alice.paskill.dialogovo.webhook.client.WebhookException;
import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Counter;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricId;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest(classes = {TestConfigProvider.class}, webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT)
class SkillIntegrationTest implements ClientInfoTestUtils {
    @Autowired
    private SkillRequestProcessor requestService;
    @Autowired
    private TestSkill testSkill;
    @Autowired
    private RequestContext requestContext;

    @Autowired
    @Qualifier("externalMetricRegistry")
    private MetricRegistry externalSensorRegistry;

    @LocalServerPort
    private int port;

    @AfterEach
    private void tearDown() {
        requestContext.setCurrentUserId(null);
    }

    SkillProcessRequest buildRequest(String utterance, String clientUuid, Set<String> featureFlags) {
        var skillInfo = TestSkills.cityGameSkill(port, featureFlags, Collections.emptyMap());
        return SkillProcessRequest.builder()
                .skill(skillInfo)
                .locationInfo(Optional.empty())
                .clientInfo(searchApp(clientUuid))
                .activationSourceType(ActivationSourceType.DIRECT)
                .session(Optional.of(Session.create(
                        "session_id",
                        1,
                        Instant.now(),
                        false,
                        Session.ProactiveSkillExitState.createEmpty(), ActivationSourceType.DIRECT)))
                .normalizedUtterance(utterance)
                .originalUtterance(utterance)
                .viewState(Optional.empty())
                .requestTime(Instant.now())
                .mementoData(RequestProto.TMementoData.getDefaultInstance())
                .build();
    }

    SkillProcessRequest buildRequest(String utterance) {
        return buildRequest(utterance, DEFAULT_UUID, Set.of());
    }

    @Test
    void test_ordinary_skill_request() {
        // given
        MetricId durationMetricId = new MetricId("request_duration",
                getLabels(TestSkills.CITY_GAME_SKILL_ID, "aliceSkill", "ok"));
        MetricId requestMetricId = new MetricId("request",
                getLabels(TestSkills.CITY_GAME_SKILL_ID, "aliceSkill", "ok"));

        long initialValue = getMetricValue(durationMetricId);
        long requestInitialValue = getMetricValue(requestMetricId);
        // when
        var response = requestService.process(new Context(SourceType.USER), buildRequest(TestSkill.COMMAND_NORMAL));

        // then
        assertEquals(initialValue + 1L, getMetricValue(durationMetricId));
        assertEquals(requestInitialValue + 1L, getMetricValue(requestMetricId));
    }

    private long getMetricValue(MetricId metricId) {
        @Nullable
        Metric metric = externalSensorRegistry.getMetric(metricId);

        if (metric == null) {
            return 0L;
        }

        switch (metric.type()) {
            case HIST:
            case HIST_RATE:
                Histogram histogram = (Histogram) metric;
                HistogramSnapshot snapshot = histogram.snapshot();
                long sum = 0;
                for (int i = 0; i < snapshot.count(); i++) {
                    sum += snapshot.value(i);
                }
                return sum;
            case RATE:
                return ((Rate) metric).get();
            case COUNTER:
                return ((Counter) metric).get();
            default:
                return 0;
        }
    }

    private Labels getLabels(String skillId, String channel, String commandStatus) {
        return Labels.of(Map.of(
                "skill_id", skillId,
                "skill_type", channel,
                "command", "webhookRequest", // TODO:
                "source", "user", // TODO: add pings and dev-console
                "api_version", "1.0", // TODO: add api_version from request
                "monitoring_type", SkillInfo.MonitoringType.NONMONITORED.getCode(),
                "command_status", commandStatus
        ));

    }

    @Test
    @Timeout(value = 20, unit = TimeUnit.SECONDS)
    void test_skill_timeout() {
        Assertions.assertThrows(WebhookException.class, () -> {
            requestService.process(new Context(SourceType.USER), buildRequest(TestSkill.COMMAND_TIMEOUT));
        });
    }

}
