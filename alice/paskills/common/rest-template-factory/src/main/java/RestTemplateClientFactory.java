package ru.yandex.alice.paskills.common.resttemplate.factory;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import org.apache.http.impl.client.CloseableHttpClient;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.client.ClientHttpRequestInterceptor;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.stereotype.Component;
import org.springframework.web.client.RestTemplate;

import ru.yandex.alice.paskills.common.logging.protoseq.ClientSetraceInterceptor;
import ru.yandex.alice.paskills.common.resttemplate.factory.client.HttpClientFactory;
import ru.yandex.alice.paskills.common.resttemplate.factory.client.interceptor.GzipCompressingClientHttpRequestInterceptor;

@Component
public class RestTemplateClientFactory {

    private final RestTemplateBuilder baseBuilder;
    private final ClientHttpHeadersInterceptor clientHttpHeadersInterceptor;
    private final HttpClientFactory httpClientFactory;

    public RestTemplateClientFactory(
            @Qualifier("baseRestTemplateBuilder") RestTemplateBuilder baseBuilder,
            ClientHttpHeadersInterceptor clientHttpHeadersInterceptor,
            HttpClientFactory httpClientFactory) {
        this.baseBuilder = baseBuilder;
        this.clientHttpHeadersInterceptor = clientHttpHeadersInterceptor;
        this.httpClientFactory = httpClientFactory;
    }

    public RestTemplate serviceWebClientWithRetry(
            String serviceName,
            Duration logWarnThreshold,
            int retryCount,
            boolean requestSentRetryEnabled,
            Duration connectTimeout,
            Duration readTimeout,
            int maxConnections
    ) {

        CloseableHttpClient client = httpClientFactory
                .builder()
                .connectTimeout(connectTimeout)
                .retryOnIO(retryCount, requestSentRetryEnabled)
                .instrumented("http.out", serviceName, false, Optional.of(logWarnThreshold))
                .maxConnectionCount(maxConnections)
                .maxConnectionsPerRoute(maxConnections)
                .build();

        return createRestTemplateWithRequestFactory(connectTimeout, readTimeout, client, false, serviceName);
    }

    public RestTemplate serviceWebClientWithRetryAndGzip(
            String serviceName,
            Duration logWarnThreshold,
            int retryCount,
            boolean requestSentRetryEnabled,
            Duration connectTimeout,
            Duration readTimeout,
            int maxConnections) {

        CloseableHttpClient client = httpClientFactory
                .builder()
                .connectTimeout(connectTimeout)
                .retryOnIO(retryCount, requestSentRetryEnabled)
                .instrumented("http.out", serviceName, false, Optional.of(logWarnThreshold))
                .maxConnectionCount(maxConnections)
                .maxConnectionsPerRoute(maxConnections)
                .build();

        return createRestTemplateWithRequestFactory(connectTimeout, readTimeout, client, true, serviceName);
    }

    public RestTemplate serviceWebClientWith5XXRetry(
            String serviceName,
            Duration logWarnThreshold,
            int retryCount,
            boolean requestSentRetryEnabled,
            Duration connectTimeout,
            Duration readTimeout,
            int maxConnections) {

        CloseableHttpClient client = httpClientFactory
                .builder()
                .connectTimeout(connectTimeout)
                .retryByIOAnd5XX(retryCount, requestSentRetryEnabled)
                .instrumented("http.out", serviceName, false, Optional.of(logWarnThreshold))
                .maxConnectionCount(maxConnections)
                .maxConnectionsPerRoute(maxConnections)
                .build();

        return createRestTemplateWithRequestFactory(connectTimeout, readTimeout, client, serviceName);
    }

    public RestTemplate serviceWebClientWithoutRetry(
            String serviceName,
            Duration logWarnThreshold,
            Duration connectTimeout,
            Duration readTimeout,
            int maxConnections,
            boolean metricsWithPath
    ) {

        CloseableHttpClient client = httpClientFactory
                .builder()
                .connectTimeout(connectTimeout)
                .disableRetry()
                .instrumented("http.out", serviceName, metricsWithPath, Optional.of(logWarnThreshold))
                .maxConnectionCount(maxConnections)
                .maxConnectionsPerRoute(maxConnections)
                .build();

        return createRestTemplateWithRequestFactory(connectTimeout, readTimeout, client, serviceName);
    }

    private RestTemplate createRestTemplateWithRequestFactory(
            Duration connectTimeout,
            Duration readTimeout,
            CloseableHttpClient client,
            String serviceName
    ) {
        return createRestTemplateWithRequestFactory(connectTimeout, readTimeout, client, false, serviceName);
    }

    private RestTemplate createRestTemplateWithRequestFactory(
            Duration connectTimeout,
            Duration readTimeout,
            CloseableHttpClient client,
            boolean gzip,
            String serviceName
    ) {
        HttpComponentsClientHttpRequestFactory requestFactory = new HttpComponentsClientHttpRequestFactory(client);

        List<ClientHttpRequestInterceptor> interceptors = new ArrayList<>();
        interceptors.add(this.clientHttpHeadersInterceptor);
        interceptors.add(new ClientSetraceInterceptor(serviceName, true));
        if (gzip) {
            interceptors.add(new GzipCompressingClientHttpRequestInterceptor());
        }

        return baseBuilder
                .requestFactory(() -> requestFactory)
                .additionalInterceptors(interceptors)
                .setReadTimeout(readTimeout)
                .setConnectTimeout(connectTimeout)
                .build();
    }
}
