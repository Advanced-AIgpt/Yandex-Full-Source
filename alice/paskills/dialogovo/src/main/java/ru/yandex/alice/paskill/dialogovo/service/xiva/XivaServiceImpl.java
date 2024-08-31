package ru.yandex.alice.paskill.dialogovo.service.xiva;

import java.net.URI;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.util.MimeTypeUtils;
import org.springframework.web.client.RestClientException;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.paskill.dialogovo.config.SecretsConfig;
import ru.yandex.alice.paskill.dialogovo.config.XivaConfig;
import ru.yandex.alice.paskill.dialogovo.processor.DirectiveToDialogUriConverter;
import ru.yandex.alice.paskill.dialogovo.utils.executor.DialogovoInstrumentedExecutorService;

class XivaServiceImpl implements XivaService {

    private static final Logger logger = LogManager.getLogger();
    private static final String SERVICE_NAME = "quasar-realtime";

    private final ParameterizedTypeReference<List<XivaSubscriptionInfo>> typeRef =
            new ParameterizedTypeReference<>() {
            };
    private final XivaConfig xivaConfig;
    private final SecretsConfig secretsConfig;
    private final RestTemplate restTemplate;
    private final DialogovoInstrumentedExecutorService executor;
    private final DirectiveToDialogUriConverter converter;

    XivaServiceImpl(XivaConfig config,
                    SecretsConfig secretsConfig,
                    RestTemplate restTemplate,
                    DialogovoInstrumentedExecutorService executor,
                    DirectiveToDialogUriConverter converter
    ) {
        this.xivaConfig = config;
        this.secretsConfig = secretsConfig;
        this.restTemplate = restTemplate;
        this.executor = executor;
        this.converter = converter;
    }

    @Override
    public void sendCallbackDirectiveAsync(
            @Nullable String userId,
            String initialDeviceId,
            CallbackDirective directive
    ) {
        if (userId == null) {
            logger.error("Trying to send xiva bush but no user_id provided. device_id = {}", initialDeviceId);
            return;
        }
        executor.runAsyncInstrumentedWithoutTimeout(
                () -> sendXivaPushToUserDevice(userId, initialDeviceId, directive));
    }

    private void sendXivaPushToUserDevice(String userId, String initialDeviceId, CallbackDirective directive) {
        List<XivaSubscriptionInfo> subscriptions = listSubscriptions(userId, SERVICE_NAME);

        Optional<String> subscriptionId = subscriptions.stream()
                .filter(sub -> sub.getSession().equals(initialDeviceId))
                .map(XivaSubscriptionInfo::getId)
                .findFirst();

        if (subscriptionId.isPresent()) {
            var wrappedDirective = converter.wrapCallbackDirective(directive);
            sendPush(userId, "server_action", subscriptionId.get(), "360", wrappedDirective);
        } else {
            logger.warn("No xiva subscription found for user {} and device {}", userId, initialDeviceId);
        }
    }

    List<XivaSubscriptionInfo> listSubscriptions(String uid, String serviceName) {
        URI uri = UriComponentsBuilder.fromUriString(xivaConfig.getUrl())
                .path("/v2/list")
                .queryParam("user", uid)
                .queryParam("service", serviceName)
                .build().toUri();

        try {
            logger.debug("Xiva subscription list for user: {}", uid);
            List<XivaSubscriptionInfo> response = restTemplate.exchange(
                    uri,
                    HttpMethod.GET,
                    new HttpEntity<>(getDefaultHeaders()),
                    typeRef).getBody();
            logger.debug("Xiva subscription list response: {}", response);
            return response != null ? response : Collections.emptyList();
        } catch (RestClientException e) {
            logger.error("Unable to obtain xiva subscriptions for user " + uid, e);
            throw e;
        }

    }

    void sendPush(String userId,
                  String event,
                  String subscriptionId,
                  @Nullable String ttl,
                  Object payload) {

        var urlBuilder = UriComponentsBuilder.fromUriString(xivaConfig.getUrl())
                .path("/v2/send")
                .queryParam("user", userId)
                .queryParam("event", event);

        if (ttl != null) {
            urlBuilder.queryParam("ttl", ttl);
        }

        var url = urlBuilder.build().toUri();

        try {
            var headers = getDefaultHeaders();
            headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE);

            var body = XivaRequest.create(payload, subscriptionId);

            logger.info("sending complete account linking push");
            logger.debug("xive send push request: {}", body);
            var res = restTemplate.exchange(url, HttpMethod.POST, new HttpEntity<>(body, headers), String.class);

            logger.debug("Xiva send push response: {}", res);
        } catch (RestClientException e) {
            logger.error("Unable to send complete account linking push", e);
            throw e;
        }
    }

    private HttpHeaders getDefaultHeaders() {
        HttpHeaders headers = new HttpHeaders();
        headers.add(HttpHeaders.AUTHORIZATION, "Xiva " + secretsConfig.getXivaToken());
        return headers;
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    private static class XivaRequest {
        private final Object payload;

        @JsonInclude(value = JsonInclude.Include.NON_EMPTY)
        @JsonProperty("subscriptions")
        private final List<SubscriptionFilter> subscriptions;

        public static XivaRequest create(Object payload, String subscriptionId) {
            return new XivaRequest(payload, subscriptionId != null ?
                    List.of(
                            new SubscriptionFilter(List.of(subscriptionId))
                    ) : Collections.emptyList());
        }
    }

    @Data
    private static class SubscriptionFilter {
        @JsonProperty("subscription_id")
        private final List<String> subscriptionId;
    }
}
