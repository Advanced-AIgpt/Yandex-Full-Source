package ru.yandex.quasar.billing.services.laas;

import java.net.SocketTimeoutException;
import java.net.URI;
import java.time.Duration;
import java.util.function.Supplier;

import org.apache.http.conn.ConnectTimeoutException;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.LaasConfig;
import ru.yandex.quasar.billing.services.AuthorizationContext;

@Component
public class LaasClientImpl implements LaasClient {

    private final RestTemplate restTemplate;
    private final AuthorizationContext context;
    private final LaasConfig config;
    private final UriComponentsBuilder baseBuilder;

    public LaasClientImpl(RestTemplateBuilder restTemplateBuilder, AuthorizationContext context,
                          BillingConfig billingConfig) {
        this.config = billingConfig.getLaasConfig();
        this.restTemplate = restTemplateBuilder
                .setReadTimeout(Duration.ofMillis(config.getReadTimeout()))
                .setConnectTimeout(Duration.ofMillis(config.getConnectTimeout()))
                .build();
        this.context = context;
        this.baseBuilder = UriComponentsBuilder.fromUriString(config.getUrl());
    }

    private static <T> T withRetry(int retryLimit, Supplier<T> call) {
        RuntimeException ex = null;
        int i = 0;
        do {
            try {
                return call.get();
            } catch (HttpServerErrorException e) {
                ex = e;
            } catch (RuntimeException e) {
                if (e.getCause() != null &&
                        (e.getCause() instanceof SocketTimeoutException ||
                                e.getCause() instanceof ConnectTimeoutException)) {
                    ex = new RuntimeException(e.getCause());
                } else {
                    throw e;
                }
            }
            i++;
        } while (i < retryLimit);
        throw ex;
    }

    RestTemplate getRestTemplate() {
        return restTemplate;
    }

    @Override
    public Integer getCountryId() {
        URI uri = baseBuilder.cloneBuilder().pathSegment("region")
                .queryParam("service", "quasar-billing")
                .queryParam("real-ip", context.getUserIp())
                .queryParam("bigb-uid", context.getYandexUid())
                .build()
                .toUri();

        return withRetry(config.getRetryLimit(), () -> restTemplate.getForObject(uri, LaasResponse.class))
                .getCountryId();
    }
}
