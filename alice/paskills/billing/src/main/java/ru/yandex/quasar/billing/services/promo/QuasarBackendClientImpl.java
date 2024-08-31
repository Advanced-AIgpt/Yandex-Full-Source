package ru.yandex.quasar.billing.services.promo;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.beans.Experiments;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.QuasarBackendConfig;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

@Component
class QuasarBackendClientImpl implements QuasarBackendClient {
    private static final Logger logger = LogManager.getLogger();
    private static final ParameterizedTypeReference<Map<String, Set<String>>> DEVICE_TO_TAGS_TYPE
            = new ParameterizedTypeReference<>() {
    };

    private final RestTemplate restTemplate;
    private final AuthorizationContext authorizationContext;
    private final TvmClient tvmClient;

    private final QuasarBackendConfig config;

    QuasarBackendClientImpl(
            RestTemplateBuilder builder,
            AuthorizationContext authorizationContext,
            TvmClient tvmClient,
            BillingConfig config) {
        this.authorizationContext = authorizationContext;
        this.tvmClient = tvmClient;
        this.config = config.getQuasarBackendConfig();
        this.restTemplate = builder
                .setConnectTimeout(Duration.ofMillis(this.config.getConnectionTimeout()))
                .setReadTimeout(Duration.ofMillis(this.config.getReadTimeout()))
                .build();
    }

    @Override
    public Collection<BackendDeviceInfo> getUserDeviceList(String uid) {
        return getWithRetry(() -> {
            ResponseEntity<DeviceListWrapper> responseEntity = restTemplate.exchange(
                    UriComponentsBuilder.fromUriString(config.getBackendUrl())
                            .pathSegment("billing", "device_list")
                            .build().toUri(),
                    HttpMethod.GET,
                    new HttpEntity<>(getHeaders()),
                    DeviceListWrapper.class
            );

            if (responseEntity.getStatusCode() == HttpStatus.OK) {
                Collection<BackendDeviceInfo> devices = responseEntity.getBody() != null ?
                        responseEntity.getBody().getDevices() : Collections.emptyList();
                if (authorizationContext.hasExperiment(Experiments.CHANGE_DEVICE_ACTIVATION_TIME_TO_WEEK_AGO)) {
                    var firstActivationTime = authorizationContext.getRequestTimestamp().minus(7, ChronoUnit.DAYS);
                    logger.info("Modifying quasar_devices first_activation_time to {}", firstActivationTime.toString());
                    return devices.stream()
                            .map(it -> it.withFirstActivationTime(firstActivationTime))
                            .collect(Collectors.toList());
                } else {
                    return devices;
                }
            } else {
                throw new QuasarBackendException(responseEntity.getStatusCode());
            }
        });
    }

    @Override
    public Map<String, Set<String>> getUserDeviceTags(String uid) {
        return getWithRetry(() -> {
            ResponseEntity<Map<String, Set<String>>> responseEntity = restTemplate.exchange(
                    UriComponentsBuilder.fromUriString(config.getBackendUrl())
                            .path("get_tags_for_user")
                            .build().toUri(),
                    HttpMethod.GET,
                    new HttpEntity<>(getHeaders()),
                    DEVICE_TO_TAGS_TYPE
            );

            if (responseEntity.getStatusCode() == HttpStatus.OK) {
                return responseEntity.getBody() != null ? responseEntity.getBody() :
                        Collections.emptyMap();
            } else {
                throw new QuasarBackendException(responseEntity.getStatusCode());
            }
        });
    }

    private HttpHeaders getHeaders() {
        var headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER,
                tvmClient.getServiceTicketFor(TvmClientName.quasar_backend.getAlias()));
        headers.add(TvmHeaders.USER_TICKET_HEADER, authorizationContext.getCurrentUserTicket());
        return headers;
    }

    private <T> T getWithRetry(Supplier<T> quasarBackendRequest) {
        int retriesCount = 1;
        while (true) {
            try {
                return quasarBackendRequest.get();
            } catch (HttpClientErrorException e) {
                throw new QuasarBackendException(e.getStatusCode(), e);
            } catch (ResourceAccessException e) {
                // SSLException: Connection reset exception occurs from time to time
                if (retriesCount++ >= config.getRetries()) {
                    throw new QuasarBackendException("Service unavailable", e);
                }
            }
        }
    }
}
