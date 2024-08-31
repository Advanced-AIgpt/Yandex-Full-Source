package ru.yandex.alice.paskill.dialogovo.service;

import java.time.Duration;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.stereotype.Component;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskill.dialogovo.config.SocialApiConfig;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;
import ru.yandex.alice.paskill.dialogovo.utils.executor.ExecutorsFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.RestTemplateClientFactory;
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders;
import ru.yandex.passport.tvmauth.TvmClient;

import static ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService.TIMEOUT_DELTA;

@Component
class SocialServiceImpl implements SocialService {

    private static final Logger logger = LogManager.getLogger();
    private static final int RETRY_COUNT = 2;

    private final RestTemplate webClient;
    private final TvmClient tvmClient;
    private final DialogovoInstrumentedExecutorService executor;
    private final SocialApiConfig endpointConfig;

    SocialServiceImpl(SocialApiConfig config,
                      RestTemplateClientFactory restTemplateClientFactory,
                      TvmClient tvmClient,
                      @Qualifier("socialServiceExecutor") DialogovoInstrumentedExecutorService executor) {

        this.endpointConfig = config;
        this.webClient = restTemplateClientFactory
                .serviceWebClientWithRetry(
                        "social",
                        Duration.ofMillis(400),
                        RETRY_COUNT, true,
                        Duration.ofMillis(endpointConfig.getConnectTimeout()),
                        Duration.ofMillis(endpointConfig.getTimeout()),
                        100);

        this.tvmClient = tvmClient;
        this.executor = executor;
    }

    @Override
    public Optional<String> getSocialToken(String userId, String oauthAppName) {
        var url = UriComponentsBuilder.fromUriString(endpointConfig.getUrl())
                .pathSegment("api/token/newest")
                .queryParam("consumer", "dialogovo")
                .queryParam("uid", userId)
                .queryParam("application_name", oauthAppName)
                .build()
                .toUri();

        try {
            var entity = getHttpEntity();
            var response = webClient.exchange(url, HttpMethod.GET, entity, Response.class);
            return Optional.of(response)
                    .map(HttpEntity::getBody)
                    .map(Response::getToken)
                    .map(Response.Token::getValue);

        } catch (HttpClientErrorException.NotFound e) {
            logger.info("No auth token for userId: {}, oauthAppName: {}", userId, oauthAppName);
        } catch (Exception e) {
            logger.error("Unable to obtain oauth token", e);
        }

        return Optional.empty();
    }

    @Override
    public CompletableFuture<Optional<String>> getSocialTokenAsync(String userId, String oauthAppName) {
        return executor.supplyAsyncInstrumented(
                () -> getSocialToken(userId, oauthAppName),
                Duration.ofMillis(RETRY_COUNT * endpointConfig.getTimeout() + TIMEOUT_DELTA)
        );
    }

    private HttpEntity getHttpEntity() {
        var ticket = tvmClient.getServiceTicketFor("social");

        var headers = new HttpHeaders();
        headers.add(SecurityHeaders.SERVICE_TICKET_HEADER, ticket);
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);
        return new HttpEntity(headers);

    }

    /**
     * https://wiki.yandex-team.ru/social/api/#tokeny
     */
    @Data
    private static class Response {
        private Token token;

        @Data
        private static class Token {
            private String value;
        }
    }

    @Configuration
    static class SocialServiceExecutorConfig {
        @Bean(value = "socialServiceExecutor", destroyMethod = "shutdownNow")
        public DialogovoInstrumentedExecutorService socialExecutorService(ExecutorsFactory executorsFactory) {
            return executorsFactory.cachedBoundedThreadPool(2, 100, 100, "social-service");
        }
    }
}
