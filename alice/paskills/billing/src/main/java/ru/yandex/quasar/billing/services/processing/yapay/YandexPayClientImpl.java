package ru.yandex.quasar.billing.services.processing.yapay;

import java.util.Objects;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.HttpServerErrorException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.YaPayConfig;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static ru.yandex.quasar.billing.services.processing.yapay.ResponseWrapper.Status.success;

class YandexPayClientImpl implements YandexPayClient {

    private static final ParameterizedTypeReference<ResponseWrapper<Order>> ORDER_TYPE =
            new ParameterizedTypeReference<>() {
            };
    private static final ParameterizedTypeReference<ResponseWrapper<StartOrderResponse>> START_ORDER_TYPE =
            new ParameterizedTypeReference<>() {
            };
    private static final ParameterizedTypeReference<ResponseWrapper<ResponseWrapper.Empty>> EMPTY_TYPE =
            new ParameterizedTypeReference<>() {
            };
    private static final ParameterizedTypeReference<ResponseWrapper<YaPayServiceMerchantInfo>>
            SERVICE_MERCHANT_INFO_TYPE = new ParameterizedTypeReference<>() {
    };
    private static final TypeReference<ResponseWrapper<MerchantAccessConflict>> SERVICE_MERCHANT_INFO_CONFLICT_TYPE =
            new TypeReference<>() {
            };
    private final RestTemplate restTemplate;
    private final TvmClient tvmClient;
    private final ObjectMapper objectMapper;
    private final YaPayConfig config;


    YandexPayClientImpl(
            BillingConfig config,
            RestTemplateBuilder restTemplateBuilder,
            TvmClient tvmClient,
            ObjectMapper objectMapper) {
        this.tvmClient = tvmClient;
        this.objectMapper = objectMapper;
        this.config = config.getYaPayConfig();
        this.restTemplate = restTemplateBuilder
                .rootUri(this.config.getApiBaseUrl())
                .build();
    }

    @Override
    public YaPayMerchant getMerchantByKey(String key) {
        HttpHeaders headers = getHttpHeaders();

        YaPayMerchant.Wrapper response = Objects.requireNonNull(
                withRetry(
                        () -> restTemplate.exchange("/v1/merchant_by_key/" + key,
                                HttpMethod.GET,
                                new HttpEntity<>(headers),
                                YaPayMerchant.Wrapper.class)
                ).getBody(),
                "YaClient response body is empty");

        return response.getData();
    }

    @Override
    public Order createOrder(long serviceMerchantId, CreateOrderRequest newOrder) {
        HttpHeaders headers = getHttpHeaders();

        return Objects.requireNonNull(
                // trailing "/" is essential for the method
                restTemplate.exchange("/v1/internal/order/{merchant_id}/",
                        HttpMethod.POST,
                        new HttpEntity<>(newOrder, headers),
                        ORDER_TYPE,
                        serviceMerchantId)
                        .getBody(),
                "YaClient response body is empty")
                .getData();
    }

    @Override
    public Order getOrder(long serviceMerchantId, long orderId) {
        HttpHeaders headers = getHttpHeaders();

        return Objects.requireNonNull(
                withRetry(
                        () -> restTemplate.exchange("/v1/internal/order/{merchant_id}/{order_id}",
                                HttpMethod.GET,
                                new HttpEntity<>(headers),
                                ORDER_TYPE,
                                serviceMerchantId, orderId)
                ).getBody(),
                "YaClient response body is empty")
                .getData();
    }

    @Override
    public StartOrderResponse startOrder(long serviceMerchantId, long orderId, StartOrderRequest startOrder) {
        HttpHeaders headers = getHttpHeaders();

        return Objects.requireNonNull(
                restTemplate.exchange("/v1/internal/order/{merchant_id}/{order_id}/start",
                        HttpMethod.POST,
                        new HttpEntity<>(startOrder, headers),
                        START_ORDER_TYPE,
                        serviceMerchantId, orderId)
                        .getBody(),
                "YaClient response body is empty")
                .getData();
    }

    @Override
    public void clearOrder(long serviceMerchantId, long orderId) {
        HttpHeaders headers = getHttpHeaders();

        Objects.requireNonNull(
                restTemplate.exchange("/v1/internal/order/{merchant_id}/{order_id}/clear",
                        HttpMethod.POST,
                        new HttpEntity<>(headers),
                        EMPTY_TYPE,
                        serviceMerchantId, orderId)
                        .getBody(),
                "YaClient response body is empty");
    }

    @Override
    public void unholdOrder(long serviceMerchantId, long orderId) {
        HttpHeaders headers = getHttpHeaders();

        Objects.requireNonNull(
                restTemplate.exchange("/v1/internal/order/{merchant_id}/{order_id}/unhold",
                        HttpMethod.POST,
                        new HttpEntity<>(headers),
                        EMPTY_TYPE,
                        serviceMerchantId, orderId)
                        .getBody(),
                "YaClient response body is empty");
    }

    @Override
    public ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description)
            throws AccessRequestConflictException, TokenNotFound {
        HttpHeaders headers = getHttpHeaders();

        MerchantAccessRequest request = new MerchantAccessRequest(token, entityId, description);

        try {
            var response = Objects.requireNonNull(
                    restTemplate.exchange("/v1/internal/service",
                            HttpMethod.POST,
                            new HttpEntity<>(request, headers),
                            SERVICE_MERCHANT_INFO_TYPE)
                            .getBody()
            );

            if (response.getStatus() == success) {
                return convert(response.getData());
            } else {
                throw new YaPayClientException("failed to request Access");
            }
        } catch (HttpClientErrorException.Conflict e) {
            long serviceMerchantId;
            try {
                ResponseWrapper<MerchantAccessConflict> value =
                        objectMapper.readValue(e.getResponseBodyAsString(), SERVICE_MERCHANT_INFO_CONFLICT_TYPE);
                serviceMerchantId = value.getData().getParams().getServiceMerchantId();
            } catch (Exception ignored) {
                throw e;
            }
            throw new AccessRequestConflictException(serviceMerchantId);
        } catch (HttpClientErrorException.NotFound e) {
            throw new TokenNotFound(e);
        }
    }

    @Override
    public ServiceMerchantInfo merchantInfo(long serviceMerchantId) {
        HttpHeaders headers = getHttpHeaders();

        var response = Objects.requireNonNull(
                withRetry(
                        () -> restTemplate.exchange("/v1/internal/service/{service_merchant_id}",
                                HttpMethod.GET,
                                new HttpEntity<>(headers),
                                SERVICE_MERCHANT_INFO_TYPE,
                                serviceMerchantId)
                ).getBody()
        );
        if (response.getStatus() == success) {
            return convert(response.getData());
        } else {
            throw new YaPayClientException("failed to request Access");
        }
    }

    private ServiceMerchantInfo convert(YaPayServiceMerchantInfo yaPayServiceMerchantInfo) {
        return new ServiceMerchantInfo(
                yaPayServiceMerchantInfo.getServiceMerchantId(),
                yaPayServiceMerchantInfo.getEntityId(),
                yaPayServiceMerchantInfo.getOrganization(),
                yaPayServiceMerchantInfo.getDescription(),
                yaPayServiceMerchantInfo.isDeleted(),
                yaPayServiceMerchantInfo.isEnabled(),
                getLegalAddress(yaPayServiceMerchantInfo)
        );
    }

    @Nullable
    private String getLegalAddress(YaPayServiceMerchantInfo yaPayServiceMerchantInfo) {
        if (yaPayServiceMerchantInfo.getAddresses() == null || yaPayServiceMerchantInfo.getAddresses().isEmpty()) {
            return null;
        }

        YaPayServiceMerchantInfo.Address address = yaPayServiceMerchantInfo.getAddresses()
                .get(YaPayServiceMerchantInfo.AddressType.legal);

        return Stream.of(
                address.getZip(),
                address.getCountry(),
                address.getCity(),
                address.getStreet(),
                address.getHome())
                .filter(Objects::nonNull)
                .collect(Collectors.joining(", "));
    }

    private <T> T withRetry(Supplier<T> call) {
        RuntimeException ex = null;
        int i = 0;
        do {
            try {
                return call.get();
            } catch (HttpServerErrorException e) {
                ex = e;
            }
            i++;
        } while (i < config.getRetryLimit());
        throw ex;
    }

    RestTemplate getRestTemplate() {
        return restTemplate;
    }

    @Nonnull
    private HttpHeaders getHttpHeaders() {
        HttpHeaders headers = new HttpHeaders();
        headers.add(TvmHeaders.SERVICE_TICKET_HEADER,
                tvmClient.getServiceTicketFor(TvmClientName.ya_pay.getAlias()));
        return headers;
    }
}
