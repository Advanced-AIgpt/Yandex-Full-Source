package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.net.URI;
import java.text.DecimalFormat;
import java.time.Duration;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.CompletionException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Collectors;

import javax.annotation.Nullable;
import javax.net.ssl.SSLException;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.exc.InvalidFormatException;
import com.fasterxml.jackson.databind.exc.MismatchedInputException;
import com.google.common.base.CaseFormat;
import com.google.common.base.Stopwatch;
import com.google.common.base.Strings;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.client.HttpStatusCodeException;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.client.UnknownHttpStatusCodeException;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.RequestContext;
import ru.yandex.alice.paskill.dialogovo.config.WebhookClientConfig;
import ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.ProtocolValidator;
import ru.yandex.alice.paskill.dialogovo.external.WebhookError;
import ru.yandex.alice.paskill.dialogovo.external.WebhookErrorCode;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequestBase;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.ResponseAnalytics;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.megamind.AliceHandledWithDevConsoleMessageException;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.SourceType;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ErrorAnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ProxyType;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.RequestSkillWebhookEvent;
import ru.yandex.alice.paskill.dialogovo.utils.client.GozoraConnectContextHolder;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.passport.tvmauth.TvmClient;

import static ru.yandex.alice.paskill.dialogovo.domain.SkillFeatureFlag.SEND_SKILL_SERVICE_TICKET_DIRECT;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.SERVICE_TICKET_HEADER;
import static ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders.USER_TICKET_HEADER;

@Component
class WebhookClientImpl implements WebhookClient {
    private static final Logger logger = LogManager.getLogger();
    private static final Logger REQUEST_LOGGER = LogManager.getLogger("WEBHOOK_REQUEST_LOGGER");
    private static final Duration DURATION_1MS = Duration.ofMillis(1);

    private final RestTemplate webhookRestTemplate;
    private final RestTemplate gozoraRestTemplate;
    private final RestTemplate gozoraPingRestTemplate;
    private final DialogovoInstrumentedExecutorService executor;
    private final ObjectMapper mapper;
    private final WebhookClientConfig config;
    private final TvmClient tvmClient;
    private final ProtocolValidator protocolValidator;
    private final RequestContext requestContext;
    private final DialogovoRequestContext dialogovoRequestContext;
    private final GozoraConnectContextHolder gozoraConnectContext;

    private static final String USER_AGENT = "Mozilla/5.0 (compatible; YandexDialogs/1.0; +http://yandex.com/bots)";

    private final String requestTimeoutHeader;
    private final String yaFunctionRequestTimeoutHeader;
    private final SolomonHelperFactory solomonHelperFactory;

    @Value("${yaFunctions.url}")
    private String yaFunctionsUrl;

    @SuppressWarnings("ParameterNumber")
    WebhookClientImpl(
            @Qualifier("webhookRestTemplate") RestTemplate webhookRestTemplate,
            @Qualifier("gozoraRestTemplate") RestTemplate gozoraRestTemplate,
            @Qualifier("gozoraPingRestTemplate") RestTemplate gozoraPingRestTemplate,
            @Qualifier("webhookExecutor") DialogovoInstrumentedExecutorService webhookExecutor,
            ObjectMapper mapper,
            WebhookClientConfig config,
            TvmClient tvmClient,
            ProtocolValidator protocolValidator,
            RequestContext requestContext,
            DialogovoRequestContext dialogovoRequestContext,
            SolomonHelperFactory solomonHelperFactory,
            GozoraConnectContextHolder gozoraConnectContext
    ) {
        this.requestContext = requestContext;
        this.dialogovoRequestContext = dialogovoRequestContext;
        this.solomonHelperFactory = solomonHelperFactory;
        this.webhookRestTemplate = webhookRestTemplate;
        this.gozoraRestTemplate = gozoraRestTemplate;
        this.gozoraPingRestTemplate = gozoraPingRestTemplate;
        this.executor = webhookExecutor;
        this.mapper = mapper;
        this.config = config;
        this.tvmClient = tvmClient;
        this.protocolValidator = protocolValidator;
        this.gozoraConnectContext = gozoraConnectContext;
        var webhookTimeout = Duration.ofMillis(config.getReadTimeout());
        this.requestTimeoutHeader = String.valueOf(
                TimeUnit.MICROSECONDS.convert(webhookTimeout.plus(Duration.ofMillis(100))));
        this.yaFunctionRequestTimeoutHeader = String.valueOf(TimeUnit.MILLISECONDS.convert(webhookTimeout));
    }

    private static String formatToSeconds(Duration duration) {
        double seconds = ((double) duration.toMillis()) / 1000;
        var decimalFormat = new DecimalFormat("#.###");
        return decimalFormat.format(seconds);
    }

    @Override
    public WebhookRequestResult callWebhook(Context context, WebhookRequestParams requestParams) {
        RequestParams httpParams = prepareHttpParams(requestParams);

        var url = httpParams.url();
        var httpClient = httpParams.httpClient();
        var headers = httpParams.headers();

        // according to RFC 8259 no for utf8 no charset has to be defined
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE);
        headers.add(HttpHeaders.USER_AGENT, USER_AGENT);
        var httpEntity = new HttpEntity<>(requestParams.getBody(), headers);

        // TODO: выпилить после того как внутренние скиллы в
        // https://a.yandex-team.ru/arc/trunk/arcadia/quality/functionality/chat_bot_lib
        // добавят заголовок Content-Type: application/json вместо Content-Type: text/html; charset=utf-8
        return postData(context, requestParams, url, httpClient, httpEntity, httpParams.proxyType);
    }

    private RequestParams prepareHttpParams(WebhookRequestParams requestParams) {
        try {
            return requestParams.isYaFunctionRequest()
                    ? prepareYaFunctionHttpRequest(requestParams)
                    : prepareExternalHttpRequest(requestParams);
        } catch (Exception cause) {
            throw new AliceHandledWithDevConsoleMessageException("Invalid skill url params",
                    ErrorAnalyticsInfoAction.REQUEST_FAILURE,
                    "Invalid skill url", cause);
        }
    }

    @SuppressWarnings("MethodLength")
    private WebhookRequestResult postData(Context context,
                                          WebhookRequestParams requestParams,
                                          URI url,
                                          RestTemplate httpClient,
                                          HttpEntity<WebhookRequestBase> httpEntity,
                                          ProxyType proxyType) {

        WebhookErrorCode code = null;
        Exception exception;
        SkillInfo skill = requestParams.getSkill();
        String body = null;
        boolean sensitiveData = false;
        var sensor = solomonHelperFactory.start(skill, requestParams.getSource(), proxyType);
        logger.info("Start make request for skill [{}] url [{}] useZora [{}] yaFunctionId [{}]",
                skill.getId(), requestParams.getWebhookUrl(), requestParams.isUseZora(), requestParams.getFunctionId());

        WebhookRequestResult responseResult = null;
        try {
            ResponseEntity<String> result = executeRequestWithTimeout(httpClient, url, httpEntity);
            sensor.stop();

            body = result.getBody();
            var headers = result.getHeaders();

            logger.info("Make request for skill {} finished at {} ms", skill.getId(),
                    sensor.getStopwatch().elapsed(TimeUnit.MILLISECONDS));

            sensor.markCommandStatusInternal("ok");
            sensor.markCommandStatusExternal("ok");
            Stopwatch sw = Stopwatch.createStarted();
            responseResult = tryParseResponse(body, sensor, requestParams.getSource(), proxyType);
            sensitiveData = responseResult.getResponse()
                    .flatMap(WebhookResponse::getAnalytics)
                    .map(ResponseAnalytics::getSensitiveData)
                    .orElse(false);

            logger.info("Parse response for skill {} resulted in success: {} (took {} ms)", skill.getId(),
                    !responseResult.hasErrors(), sw.stop().elapsed().toMillis());

            return responseResult;
        } catch (ResourceAccessException | TimeoutException e) {
            sensor.stop();
            exception = e;
            if (e.getCause() instanceof ResponseLimitingInterceptor.ResponseTooLargeException) {
                sensor.markCommandStatusExternal("error_response_body_size");
                sensor.markCommandStatusInternal("error_response_body_size");
                code = WebhookErrorCode.RESPONSE_TO_LONG;
            } else if (e.getCause() instanceof SSLException) {
                logger.error("Failed to call webhook with ssl exception", e.getCause());
                // don't show ssl cause to public
                sensor.markCommandStatusExternal("error_other");
                sensor.markCommandStatusInternal("error_http_ssl");
                code = WebhookErrorCode.INVALID_SSL;
            } else {
                logger.warn("Webhook call exception", e);
                sensor.markCommandStatusExternal("error_http_timeout");
                sensor.markCommandStatusInternal("error_http_timeout");
                code = WebhookErrorCode.TIME_OUT;
            }
        } catch (HttpStatusCodeException e) {
            sensor.stop();
            exception = e;
            String responseBodyAsString = e.getResponseBodyAsString();
            try {
                logger.warn("Skill {} failed with code: {} and body: {}",
                        skill.getId(), e.getStatusCode(), responseBodyAsString);
            } catch (Exception ignore) {
                // ignore exception on log
            }

            code = handleHttpStatusCodeError(e, sensor);
            if (proxyType == ProxyType.GOZORA) {
                logger.warn("Gozora request failed with status={}", e.getStatusText());
            }
        } catch (UnknownHttpStatusCodeException e) {
            exception = e;
            if (e.getRawStatusCode() == 599 &&
                    e.getResponseHeaders() != null &&
                    e.getResponseHeaders().containsKey(GoZoraResponseHeaders.ERROR_CODE)
            ) {
                String errorCode = e.getResponseHeaders().getFirst(GoZoraResponseHeaders.ERROR_CODE);
                logger.error("Gozora request failed with status={}, code={}, description={}",
                        e.getStatusText(),
                        errorCode,
                        e.getResponseHeaders().getFirst(GoZoraResponseHeaders.ERROR_DESCRIPTION));
                if (GoZoraErrors.INVALID_SSL.equals(errorCode)) {
                    sensor.markCommandStatusExternal("error_http_5xx");
                    sensor.markCommandStatusInternal("error_wrong_certificate");
                    code = WebhookErrorCode.INVALID_SSL;
                } else {
                    sensor.markCommandStatusExternal("error_other");
                    sensor.markCommandStatusInternal("error_other");
                    code = WebhookErrorCode.UNKNOWN;
                }
            } else {
                if (proxyType == ProxyType.GOZORA) {
                    logger.error("Gozora request failed with status={}", e.getStatusText());
                }
                sensor.stop();
                sensor.markCommandStatusExternal("error_other");
                sensor.markCommandStatusInternal("error_other");
                code = WebhookErrorCode.UNKNOWN;
                logger.error("Make request for skill failed with unknown exception", e);
            }
        } catch (Exception e) {
            sensor.stop();
            exception = e;
            sensor.markCommandStatusExternal("error_other");
            sensor.markCommandStatusInternal("error_other");
            code = WebhookErrorCode.UNKNOWN;
            logger.error("Make request for skill failed with unknown exception", e);
        } finally {
            if (!sensitiveData) {
                REQUEST_LOGGER.debug("calling webhook {} : {}", url::toASCIIString,
                        () -> toString(httpEntity.getBody(), requestParams.getAuthToken()));
            } else {
                REQUEST_LOGGER.debug("calling webhook {} : request body logging skipped due to sensitive data",
                        url.toASCIIString());
            }
            logResponseBody(body, sensitiveData, requestParams);
            sensor.complete();
            Duration duration = sensor.getStopwatch().elapsed();
            saveAnalyticsInfoEvent(context, url.toASCIIString(),
                    // in tests stubbed call may take less than 1 ms and be discarded from proto as 0 is default value
                    maxOf(duration, DURATION_1MS),
                    responseResult, code, proxyType);
            dialogovoRequestContext.setWebhookRequestDurationMs(duration.toMillis());
        }

        logger.error("Make request for skill {} failed with {}", skill.getId(), code);

        return error(code, Optional.of(exception), sensor, proxyType);
    }

    private ResponseEntity<String> executeRequestWithTimeout(
            RestTemplate httpClient,
            URI url,
            HttpEntity<?> httpEntity
    ) throws Exception {
        var gozoraContextValue = gozoraConnectContext.getContext();
        try {
            return executor.supplyAsyncInstrumented(
                    () -> {
                        gozoraConnectContext.setContext(gozoraContextValue);
                        return httpClient.exchange(url, HttpMethod.POST, httpEntity, String.class);
                    },
                    Duration.ofMillis(config.getFullRequestTimeout())
            ).get();
        } catch (Exception e) {
            // unwrap exception from Executor wrappers
            while ((e instanceof CompletionException || e instanceof ExecutionException) &&
                    e.getCause() != null &&
                    e.getCause() instanceof Exception
            ) {
                e = (Exception) e.getCause();
            }
            throw e;
        } finally {
            gozoraConnectContext.clear();
        }
    }

    private void logResponseBody(@Nullable String result, boolean sensitive, WebhookRequestParams requestParams) {
        if (sensitive) {
            REQUEST_LOGGER.debug("Webhook response logging skipped due to sensitive data");
        } else if (result != null && requestParams.getAuthToken() != null) {
            var escapedToken = requestParams.getAuthToken().replace("\"", "\\\"");
            REQUEST_LOGGER.debug("Webhook response: {}",
                    result.replace(escapedToken, "*".repeat(requestParams.getAuthToken().length())));
        } else {
            REQUEST_LOGGER.debug("Webhook response: {}", result);
        }
    }

    private Duration maxOf(Duration v1, Duration v2) {
        return v1.compareTo(v2) < 0 ? v2 : v1;
    }

    private void saveAnalyticsInfoEvent(
            Context context,
            String url,
            Duration duration,
            @Nullable WebhookRequestResult responseResult,
            @Nullable WebhookErrorCode failureCode,
            ProxyType proxyType
    ) {
        if (failureCode != null) {
            context.getAnalytics()
                    .addEvent(RequestSkillWebhookEvent.error(
                            url,
                            duration,
                            failureCode.getCode(),
                            failureCode.getDescription(),
                            proxyType));
            // responseResult must be not null when failureCode is null
        } else if (!responseResult.hasErrors()) {
            context.getAnalytics()
                    .addEvent(RequestSkillWebhookEvent.success(url, duration, proxyType));
        } else {
            List<RequestSkillWebhookEvent.Error> errors = responseResult.getErrors().stream()
                    .map(err ->
                            new RequestSkillWebhookEvent.Error(
                                    err.code().getCode(),
                                    Objects.requireNonNullElse(err.message(), err.code().getDescription()) +
                                            Optional.ofNullable(err.path()).map(p -> " (" + p + ")").orElse("")
                            ))
                    .collect(Collectors.toList());
            context.getAnalytics()
                    .addEvent(RequestSkillWebhookEvent.error(url, duration, errors, proxyType));
        }
    }

    private String toString(@Nullable Object body, @Nullable String token) {
        if (body == null) {
            return "";
        }
        try {
            String valueAsString = mapper.writeValueAsString(body);
            if (token != null) {
                var escapedToken = token.replace("\"", "\\\"");
                valueAsString = valueAsString.replace(escapedToken, "*".repeat(token.length()));
            }
            return valueAsString;
        } catch (JsonProcessingException e) {
            logger.error(e.getMessage(), e);
            return "";
        }
    }

    private WebhookErrorCode handleHttpStatusCodeError(HttpStatusCodeException e, SolomonHelper sensor) {
        var responseHeaders = e.getResponseHeaders();
        var statusCode = e.getStatusCode().value();
        logger.info("Make request for skill failed with http code [{}]", statusCode);
        WebhookErrorCode webhookErrorCode;
        if (300 <= statusCode && statusCode < 400) {
            sensor.markCommandStatusExternal("error_http_3xx");
            sensor.markCommandStatusInternal("error_http_3xx");
            webhookErrorCode = WebhookErrorCode.HTTP_ERROR_300;
        } else if (statusCode < 500) {
            sensor.markCommandStatusExternal("error_http_4xx");
            sensor.markCommandStatusInternal("error_http_4xx");
            webhookErrorCode = WebhookErrorCode.HTTP_ERROR_400;
        } else {
            sensor.markCommandStatusExternal("error_http_5xx");
            sensor.markCommandStatusInternal("error_http_5xx");
            webhookErrorCode = WebhookErrorCode.HTTP_ERROR_500;
        }
        return webhookErrorCode;
    }

    private WebhookRequestResult error(
            WebhookErrorCode code,
            Optional<Exception> exception,
            SolomonHelper sensor,
            ProxyType proxyType
    ) {
        var errors = List.of(WebhookError.create(code));
        return new WebhookRequestResult(
                Optional.empty(),
                Optional.empty(),
                errors,
                exception,
                sensor.getStopwatch().elapsed(),
                proxyType
        );
    }

    private WebhookRequestResult tryParseResponse(
            @Nullable String responseBody,
            SolomonHelper sensor,
            SourceType source,
            ProxyType proxyType
    ) {
        if (responseBody == null) {
            sensor.markCommandStatusExternal("error_parse");
            sensor.markCommandStatusInternal("error_parse");
            return error(WebhookErrorCode.EMPTY_RESPONSE, Optional.empty(), sensor, proxyType);
        }

        WebhookErrorCode code;
        String path;
        Exception exception;

        try {
            var webhookResponse = mapper.readValue(responseBody, WebhookResponse.class);
            var validationResult = protocolValidator.validate(webhookResponse, source);
            if (!validationResult.isValid()) {
                sensor.markCommandStatusExternal("error_validation");
                sensor.markCommandStatusInternal("error_validation");
                logger.info("validation errors: {}", validationResult.errors());
            }

            return new WebhookRequestResult(
                    Optional.of(responseBody),
                    Optional.of(webhookResponse),
                    validationResult.errors(),
                    Optional.empty(),
                    sensor.getStopwatch().elapsed(),
                    proxyType);

        } catch (InvalidFormatException e) {
            exception = e;
            path = getPath(e);
            code = WebhookErrorCode.INVALID_VALUE;
        } catch (MismatchedInputException e) {
            exception = e;
            // dirty hack - there can be JsonTypeInfo properties not only with property_name=type
            path = getPath(e, "type");
            code = WebhookErrorCode.TYPE_MISMATCH;
        } catch (JsonMappingException e) {
            exception = e;
            path = getPath(e);
            code = WebhookErrorCode.INVALID_RESPONSE;
        } catch (Exception e) {
            exception = e;
            path = "";
            code = WebhookErrorCode.INVALID_RESPONSE;
        }

        sensor.markCommandStatusExternal("error_parse");
        sensor.markCommandStatusInternal("error_parse");
        logger.info(() -> "validation error: " + exception.getMessage(), exception);

        var errors = List.of(WebhookError.create(path, code));
        return new WebhookRequestResult(
                Optional.of(responseBody),
                Optional.empty(),
                errors,
                Optional.of(exception),
                sensor.getStopwatch().elapsed(),
                proxyType);
    }

    private String getPath(JsonMappingException e, @Nullable String... subPaths) {
        var path = new StringBuilder();
        for (var seg : e.getPath()) {
            if (seg.getIndex() > -1) {
                path.append("[");
                path.append(seg.getIndex());
                path.append("]");
            } else {
                path.append(".");
                path.append(CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, seg.getFieldName()));
            }
        }

        if (subPaths != null) {
            Arrays.stream(subPaths).forEach(subPath -> path.append(".").append(subPath));
        }

        path.deleteCharAt(0);
        return path.toString();
    }

    private WebhookClientImpl.RequestParams prepareYaFunctionHttpRequest(WebhookRequestParams requestParams) {
        final String schema = "https";
        final ProxyType proxyType = ProxyType.GOZORA;
        final RestTemplate restTemplate = getGozoraRestTemplate(requestParams.getSource());

        var url = UriComponentsBuilder
                .fromHttpUrl(yaFunctionsUrl)
                .scheme(schema)
                .pathSegment(requestParams.getFunctionId())
                .queryParam("integration", "raw")
                .build()
                .toUri();

        var serviceTicket = tvmClient.getServiceTicketFor("cloud");
        var headers = new HttpHeaders();
        headers.add("X-Functions-Timeout-Ms", yaFunctionRequestTimeoutHeader);
        headers.add("X-Functions-Service-Ticket", serviceTicket);

        return new RequestParams(url, headers, restTemplate, proxyType);
    }

    private WebhookClientImpl.RequestParams prepareExternalHttpRequest(WebhookRequestParams requestParams) {
        String webhookUrl = Objects.requireNonNull(requestParams.getWebhookUrl(),
                "webhook URL may be null only for Cloud functions skill");
        var urlBuilder = UriComponentsBuilder.fromUriString(webhookUrl);

        var headers = new HttpHeaders();
        headers.add("request-timeout", requestTimeoutHeader);

        final RestTemplate httpClient;
        final ProxyType proxyType;
        if (!requestParams.isUseZora()) {
            logger.info("Don't use zora for webhook request");
            httpClient = webhookRestTemplate;
            proxyType = ProxyType.DIRECT;

            if (requestParams.getSkill().hasFeatureFlag(SEND_SKILL_SERVICE_TICKET_DIRECT)) {
                String serviceTicket = tvmClient.getServiceTicketFor("direct_skill");
                headers.add(SERVICE_TICKET_HEADER, serviceTicket);
                if (!Strings.isNullOrEmpty(requestContext.getCurrentUserTicket())) {
                    headers.add(USER_TICKET_HEADER, requestContext.getCurrentUserTicket());
                }
            }

        } else {
            logger.info("Use gozora for webhook request");
            httpClient = getGozoraRestTemplate(requestParams.getSource());
            // gozora will filter these headers out
            // we need to set headers twice (in gozoraConnectContext and headers variables)
            // for CONNECT and POST requests
            var requestId = requestParams.getInternalRequestId();
            headers.add(GoZoraRequestHeaders.REQUEST_ID, requestId);
            headers.add(
                    GoZoraRequestHeaders.CLIENT_ID,
                    GozoraClientId.fromRequestSource(requestParams.getSource()).value()
            );
            if (requestParams.getSkill().hasFeatureFlag(SkillFeatureFlag.IGNORE_INVALID_SSL)) {
                headers.add(GoZoraRequestHeaders.IGNORE_SSL_ERRORS, "true");
                gozoraConnectContext.setContext(requestId, true);
            } else {
                gozoraConnectContext.setContext(requestId, false);
            }
            proxyType = ProxyType.GOZORA;
        }

        // https://st.yandex-team.ru/PASKILLS-4190
        // мы не можем ходить с этим заголовком в лямбду функцию
        var authToken = requestParams.getAuthToken();
        if (authToken != null && !authToken.isEmpty()) {
            // Temporary hardcode until Golfstrim changes their skill
            // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/external_skill/skill.cpp?rev=5829163#L1015
            var prefix = requestParams.getSkill().getId().equals("7bf49045-c077-430a-938c-10858843dfed")
                    ? "OAuth "
                    : "Bearer ";

            headers.set(HttpHeaders.AUTHORIZATION, prefix + authToken);
        }

        var url = urlBuilder.build().toUri();
        return new RequestParams(url, headers, httpClient, proxyType);
    }

    private RestTemplate getGozoraRestTemplate(SourceType sourceType) {
        if (sourceType == SourceType.PING) {
            return gozoraPingRestTemplate;
        } else {
            return gozoraRestTemplate;
        }
    }

    private record RequestParams(URI url, HttpHeaders headers, RestTemplate httpClient, ProxyType proxyType) {
    }

    @Configuration
    public static class ExecutorConfiguration {
        @Bean(value = "webhookExecutor", destroyMethod = "shutdownNow")
        public DialogovoInstrumentedExecutorService nerExecutorService(ExecutorsFactory executorsFactory) {
            return executorsFactory.cachedBoundedThreadPool(10, 100, 100, "webhook");
        }
    }
}
