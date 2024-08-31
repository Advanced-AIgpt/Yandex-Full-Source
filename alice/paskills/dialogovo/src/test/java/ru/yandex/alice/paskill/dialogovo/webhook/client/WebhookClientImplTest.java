package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider;
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.WebhookErrorCode;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.Nlu;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.Markup;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.SimpleUtteranceRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.UtteranceWebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequestBase;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.test.TestSkills;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static java.util.Collections.emptyList;
import static java.util.Collections.emptyMap;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;

@SpringBootTest(classes = {
        TestConfigProvider.class,
        E2EConfigProvider.class,
        WebhookClientImplTest.WebhookClientTestConfiguration.class,
}
)
class WebhookClientImplTest {

    private static final String REQUEST_ID = "3a15f07d-123a-43c8-9870-cf5b8cd2c7cf";

    @Autowired
    private WebhookClientImpl webhookClient;

    @Autowired
    @Qualifier("webhookRestTemplate")
    private RestTemplate webhookRestTemplate;

    @Autowired
    @Qualifier("gozoraRestTemplate")
    private RestTemplate gozoraRestTemplate;

    @Autowired
    @Qualifier("gozoraPingRestTemplate")
    private RestTemplate gozoraPingRestTemplate;

    private Context context;
    public static final SkillInfo SKILL_USE_ZORA = TestSkills.secondMemorySkill(
            0,
            true,
            true,
            Collections.emptySet(),
            emptyMap()
    );

    @Qualifier("externalMetricRegistry")
    @SpyBean
    private MetricRegistry externalSensorRegistry;

    @Qualifier("internalMetricRegistry")
    @SpyBean
    private MetricRegistry internalSensorRegistry;

    @TestConfiguration("webhookRestTemplateConfiguration")
    static class WebhookClientTestConfiguration {

        @Bean("webhookRestTemplate")
        RestTemplate mockRestTemplate() {
            return Mockito.mock(RestTemplate.class);
        }

        @Bean("gozoraRestTemplate")
        RestTemplate gozoraRestTemplate() {
            return Mockito.mock(RestTemplate.class);
        }

        @Bean("gozoraPingRestTemplate")
        RestTemplate gozoraPingRestTemplate() {
            return Mockito.mock(RestTemplate.class);
        }

    }

    @BeforeEach
    void setup() {
        context = new Context(SourceType.USER);
        Mockito.reset(webhookRestTemplate,
                gozoraRestTemplate,
                gozoraPingRestTemplate);
    }

    WebhookRequestBase makeBody(String skillId) {
        var request = new SimpleUtteranceRequest(
                "ping",
                "ping",
                new Nlu(List.of("ping"), emptyList(), emptyMap()),
                Markup.of(false));
        Meta meta = Meta.builder()
                .clientId("ru.yandex.searchplugin/7.16 (none none; android 4.4.2)")
                .interfaces(new Meta.Interfaces(
                        Optional.of(Meta.Interfaces.Screen.INSTANCE),
                        Optional.empty(),
                        Optional.of(Meta.Interfaces.AccountLinking.INSTANCE),
                        Optional.empty(),
                        Optional.empty(),
                        Optional.empty(),
                        Optional.of(Meta.Interfaces.UserAgreements.INSTANCE)
                ))
                .build();
        WebhookSession session = WebhookSession.builder()
                .messageId(0)
                .sessionId(UUID.randomUUID().toString())
                .skillId(skillId)
                .application(new WebhookSession.Application(
                        "ACDBC9CA80FB400687FEA58B8DB5BB9D411551C3C8138BAD1A8A84F37B2E0487"))
                .build();
        return new UtteranceWebhookRequest(meta, session, request, null);
    }

    private WebhookRequestParams makeParams(SkillInfo skill, SourceType sourceType) {

        return WebhookRequestParams.builder(skill, sourceType, makeBody(skill.getId()))
                .internalRequestId(REQUEST_ID)
                .build();
    }

    private void assertRestTemplateInvocationCount(int noZora, int zora, int pingZora) {
        assertEquals(noZora, Mockito.mockingDetails(webhookRestTemplate).getInvocations().size());
        assertEquals(zora, Mockito.mockingDetails(gozoraRestTemplate).getInvocations().size());
        assertEquals(pingZora, Mockito.mockingDetails(gozoraPingRestTemplate).getInvocations().size());
    }

    private void assertSensorLabelCommandStatus(String commandStatus) {
        var invocations = Mockito.mockingDetails(externalSensorRegistry).getInvocations();
        var rateInvocation = invocations.stream()
                .filter(i -> "rate".equals(i.getMethod().getName()))
                .findFirst();
        assertTrue(rateInvocation.isPresent());
        Labels labels = rateInvocation.get().getArgument(1);
        assertEquals(labels.findByKey("command_status").getValue(), commandStatus);
    }

    private void assertInvocationHasZoraHeaders(RestTemplate restTemplateMock) {
        var invocations = Mockito.mockingDetails(restTemplateMock).getInvocations();
        var firstInvocation = invocations.stream().findFirst().get();
        String reqIdHeader = null;
        for (var argument : firstInvocation.getArguments()) {
            if (argument instanceof HttpEntity) {
                HttpEntity httpEntity = (HttpEntity) argument;
                reqIdHeader = httpEntity.getHeaders().getFirst(GoZoraRequestHeaders.REQUEST_ID);
            }
        }
        assertEquals(REQUEST_ID, reqIdHeader);
    }

    @Test
    void testUseSeparateZoraSourceForPings() {
        WebhookRequestParams params = makeParams(SKILL_USE_ZORA, SourceType.PING);
        Mockito.when(gozoraPingRestTemplate.exchange(any(), any(), any(), any(Class.class)))
                .thenReturn(new ResponseEntity("", HttpStatus.OK));
        webhookClient.callWebhook(context, params);
        assertRestTemplateInvocationCount(0, 0, 1);
        assertInvocationHasZoraHeaders(gozoraPingRestTemplate);
    }

    @Test
    void testUseMainZoraSourceForUserRequests() {
        WebhookRequestParams params = makeParams(SKILL_USE_ZORA, SourceType.USER);
        Mockito.when(gozoraRestTemplate.exchange(any(), any(), any(), any(Class.class)))
                .thenReturn(new ResponseEntity("", HttpStatus.OK));
        webhookClient.callWebhook(context, params);
        assertRestTemplateInvocationCount(0, 1, 0);
    }

    @Test
    void testUseMainZoraSourceForDeveloperRequests() {
        WebhookRequestParams params = makeParams(SKILL_USE_ZORA, SourceType.CONSOLE);
        Mockito.when(gozoraRestTemplate.exchange(any(), any(), any(), any(Class.class)))
                .thenReturn(new ResponseEntity("", HttpStatus.OK));
        webhookClient.callWebhook(context, params);
        assertRestTemplateInvocationCount(0, 1, 0);
        assertInvocationHasZoraHeaders(gozoraRestTemplate);
    }

    @Test
    void testDoNotUseZoraOnUseZoraFalse() {
        final SkillInfo skill = TestSkills.secondMemorySkill(
                0,
                true,
                false,
                Collections.emptySet(),
                emptyMap()
        );
        WebhookRequestParams params = makeParams(skill, SourceType.USER);
        Mockito.when(webhookRestTemplate.exchange(any(), any(), any(), any(Class.class)))
                .thenReturn(new ResponseEntity("", HttpStatus.OK));
        webhookClient.callWebhook(context, params);
        assertRestTemplateInvocationCount(1, 0, 0);
    }

    private void assertErrorHasRightCodeAndSolomonMetrics(
            WebhookRequestResult result,
            WebhookErrorCode expectedErrorCode,
            String expectedSolomonCommandStatus
    ) {
        assertTrue(result.getErrors().size() > 0);
        var error = result.getErrors().get(0);
        assertEquals(error.code(), expectedErrorCode);
        assertSensorLabelCommandStatus(expectedSolomonCommandStatus);
    }

    private void assertErrorHasRightCode(
            WebhookRequestResult result,
            WebhookErrorCode expectedErrorCode
    ) {
        assertTrue(result.getErrors().size() > 0);
        var error = result.getErrors().get(0);
        assertEquals(error.code(), expectedErrorCode);
    }

    @Test
    void treatResourceAccessExceptionAsTimeout() {
        when(gozoraRestTemplate.exchange(any(), any(), any(), any(Class.class)))
                .thenThrow(new ResourceAccessException("timeout"));
        WebhookRequestParams params = makeParams(SKILL_USE_ZORA, SourceType.USER);
        var result = webhookClient.callWebhook(context, params);
        assertErrorHasRightCodeAndSolomonMetrics(result, WebhookErrorCode.TIME_OUT, "error_http_timeout");
    }

    @Test
    void treat504WithoutZoraErrorsAs5xx() {
        var exception = HttpServerErrorException.create(
                HttpStatus.GATEWAY_TIMEOUT,
                "Gateway timeout",
                new HttpHeaders(),
                new byte[1],
                null);
        when(gozoraRestTemplate.exchange(any(), any(), any(), any(Class.class))).thenThrow(exception);
        WebhookRequestParams params = makeParams(SKILL_USE_ZORA, SourceType.USER);
        var result = webhookClient.callWebhook(context, params);
        assertErrorHasRightCodeAndSolomonMetrics(result, WebhookErrorCode.HTTP_ERROR_500, "error_http_5xx");
    }
}
