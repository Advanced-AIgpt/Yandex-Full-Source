package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.time.Duration;
import java.util.Optional;

import org.apache.http.impl.client.CloseableHttpClient;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskill.dialogovo.utils.client.GozoraHttpClientFactory;
import ru.yandex.alice.paskills.common.logging.protoseq.ClientSetraceInterceptor;

@Configuration
class WebhookRestTemplateConfiguration {

    private final RestTemplateBuilder baseBuilder;
    private final ResponseLimitingInterceptor responseLimitingInterceptor;
    private final GozoraHttpClientFactory httpClientFactory;

    private static final int MAX_CONNECTIONS_TOTAL = 300;
    private static final int MAX_CONNECTIONS_PER_ROUTE = 300;

    @Value("${webhookClientConfig.connectTimeout}")
    private int connectTimeout;
    @Value("${webhookClientConfig.readTimeout}")
    private int readTimeout;

    WebhookRestTemplateConfiguration(
            ResponseLimitingInterceptor responseLimitingInterceptor,
            @Qualifier("baseRestTemplateBuilder") RestTemplateBuilder baseBuilder,
            GozoraHttpClientFactory httpClientFactory
    ) {
        this.responseLimitingInterceptor = responseLimitingInterceptor;
        this.httpClientFactory = httpClientFactory;
        this.baseBuilder = baseBuilder;
    }

    @Bean("webhookRestTemplate")
    public RestTemplate webhookRestTemplate() {
        CloseableHttpClient client = httpClientFactory
                .builder()
                .connectTimeout(Duration.ofMillis(this.connectTimeout))
                // as we use only POST requests with `false` flag we can retry connection exceptions
                // which occur even before we transfer request body to the target
                .retryOnIO(2, false)
                .instrumented("http.out", "webhook", true, Optional.empty())
                .maxConnectionCount(MAX_CONNECTIONS_TOTAL)
                .maxConnectionsPerRoute(MAX_CONNECTIONS_PER_ROUTE)
                .disableRedirects()
                .disableCookieManagement()
                .build();

        HttpComponentsClientHttpRequestFactory requestFactory = new HttpComponentsClientHttpRequestFactory(client);

        return baseBuilder
                .requestFactory(() -> requestFactory)
                .additionalInterceptors(
                        responseLimitingInterceptor,
                        new ClientSetraceInterceptor("webhook.no_zora", false))
                .setReadTimeout(Duration.ofMillis(this.readTimeout))
                .setConnectTimeout(Duration.ofMillis(this.connectTimeout))
                .build();
    }

    public RestTemplate createGozoraRestTemplate(GozoraClientId gozoraClientId) {
        CloseableHttpClient client = httpClientFactory
                .builderForGozora(gozoraClientId)
                .connectTimeout(Duration.ofMillis(this.connectTimeout))
                // as we use only POST requests with `false` flag we can retry connection exceptions
                // which occur even before we transfer request body to the target
                .retryOnIO(2, false)
                .instrumented("http.out", "webhook.gozora", false, Optional.empty())
                .maxConnectionCount(MAX_CONNECTIONS_TOTAL)
                .maxConnectionsPerRoute(MAX_CONNECTIONS_PER_ROUTE)
                .disableRedirects()
                .disableCookieManagement()
                .build();

        HttpComponentsClientHttpRequestFactory requestFactory = new HttpComponentsClientHttpRequestFactory(client);

        return baseBuilder
                .requestFactory(() -> requestFactory)
                .additionalInterceptors(responseLimitingInterceptor)
                .setReadTimeout(Duration.ofMillis(this.readTimeout))
                .setConnectTimeout(Duration.ofMillis(this.connectTimeout))
                .additionalInterceptors(
                        new ClientSetraceInterceptor("webhook.gozora", false))
                .build();
    }

    @Bean("gozoraRestTemplate")
    public RestTemplate gozoraRestTemplate() {
        return createGozoraRestTemplate(GozoraClientId.DEFAULT);
    }

    @Bean("gozoraPingRestTemplate")
    public RestTemplate gozoraPingRestTemplate() {
        return createGozoraRestTemplate(GozoraClientId.PING);
    }

}
