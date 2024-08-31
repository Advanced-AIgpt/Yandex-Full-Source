package ru.yandex.quasar.billing.services.processing.trust;

import java.time.Duration;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.util.MultiValueMap;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;

import static ru.yandex.quasar.billing.services.tvm.TvmHeaders.SERVICE_TICKET_HEADER;

class TrustBillingClientImpl implements TrustBillingClient {
    private final RestTemplate restTemplate;
    private final TvmClient tvmClient;
    private final boolean useTvm;
    private final String serviceToken;
    private final String tvmClientId;

    @SuppressWarnings("ParameterNumber")
    TrustBillingClientImpl(
            String baseUrl,
            RestTemplateBuilder restTemplateBuilder,
            TvmClient tvmClient,
            long connectTimeout,
            long timeout,
            String serviceToken,
            String tvmClientId,
            boolean useTvm
    ) {
        this.restTemplate = restTemplateBuilder.rootUri(baseUrl)
                .setConnectTimeout(Duration.ofMillis(connectTimeout))
                .setReadTimeout(Duration.ofMillis(timeout))
                .build();
        this.tvmClient = tvmClient;
        this.serviceToken = serviceToken;
        this.tvmClientId = tvmClientId;
        this.useTvm = useTvm;
    }

    RestTemplate getRestTemplate() {
        return restTemplate;
    }

    @Override
    public CreateBasketResponse createBasket(String uid, String userIp, CreateBasketRequest createBasketRequest) {
        MultiValueMap<String, String> headers = getHeaders(uid, userIp);
        headers.add("Content-Type", "application/json");

        //TODO: обработка кода ошибки от trust-а
        return restTemplate.postForObject("/v2/payments",
                new HttpEntity<>(createBasketRequest, headers),
                CreateBasketResponse.class
        );
    }

    // запускаем корзину на оплату
    @Override
    public void startPayment(String uid, String userIp, String purchaseToken) {

        MultiValueMap<String, String> headers = getHeaders(uid, userIp);


        //TODO: обработка кода ошибки от trust-а
        ResponseEntity<String> entity2 = restTemplate.postForEntity("/v2/payments/{purchaseToken}/start",
                new HttpEntity<>(null, headers),
                String.class,
                purchaseToken
        );
    }

    @Override
    public TrustPaymentShortInfo getPaymentShortInfo(String uid, String userIp, String purchaseToken) {
        MultiValueMap<String, String> headers = getHeaders(uid, userIp);

        ResponseEntity<TrustPaymentShortInfo> response = restTemplate.exchange("/v2/payments/{purchaseToken}",
                HttpMethod.GET,
                new HttpEntity<>(null, headers),
                TrustPaymentShortInfo.class,
                purchaseToken
        );

        //TODO: обработка кода ошибки от trust-а

        return response.getBody();
    }

    @Override
    public void clearPayment(
            String uid,
            String userIp,
            String purchaseToken
    ) {
        MultiValueMap<String, String> headers = getHeaders(uid, userIp);

        ResponseEntity<String> entity2 = restTemplate.postForEntity("/v2/payments/{purchaseToken}/clear",
                new HttpEntity<>(null, headers),
                String.class,
                purchaseToken
        );

        //TODO: обработка кода ошибки от trust-а
    }

    @Override
    public void unholdPayment(
            String uid,
            String userIp,
            String purchaseToken
    ) {
        MultiValueMap<String, String> headers = getHeaders(uid, userIp);

        ResponseEntity<String> entity = restTemplate.postForEntity("/v2/payments/{purchaseToken}/unhold",
                new HttpEntity<>(null, headers),
                String.class,
                purchaseToken
        );

        //TODO: обработка кода ошибки от trust-а
    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, String userIp) {
        MultiValueMap<String, String> headers = getHeaders(uid, userIp);

        ResponseEntity<CardInfoWrapper> entity2 = restTemplate.exchange("/v2/payment-methods?payment-method=card",
                HttpMethod.GET,
                new HttpEntity<>(null, headers),
                CardInfoWrapper.class
        );


        if (entity2.getStatusCode() != HttpStatus.OK) {
            throw new HttpClientErrorException(entity2.getStatusCode());
        } else if (!"success".equals(entity2.getBody().status())) {
            // TODO: ProcessingException shouldn't be used on that layer, its the service-layer exception
            throw new ProcessingException("payment-methods failure. status: " + entity2.getBody().status());
        }

        //TODO: обработка кода ошибки от trust-а

        return entity2.getBody().boundPaymentMethods();
    }

    @Override
    public String createBinding(String uid, String userIp, TrustCurrency currency, String backUrl,
                                TemplateTag template) {
        HttpHeaders headers = getHeaders(uid, userIp);
        headers.add("Content-Type", "application/json");

        // TODO: научиться открывать декстопную
        CreateBindingRequest createBindingRequest = new CreateBindingRequest(currency, null, backUrl, template);

        ResponseEntity<CreateBindingResponse> response =
                restTemplate.postForEntity("/v2/bindings/",
                        new HttpEntity<>(createBindingRequest, headers),
                        CreateBindingResponse.class);
        return response.getBody().getPurchaseToken();
    }

    @Override
    public String startBinding(String uid, String userIp, String purchaseToken) {
        HttpHeaders headers = getHeaders(uid, userIp);

        ResponseEntity<StartBindingResponse> response =
                restTemplate.postForEntity("/v2/bindings/{purchase_token}/start",
                        new HttpEntity<>(headers),
                        StartBindingResponse.class,
                        purchaseToken);
        return response.getBody().getBindingUrl();
    }

    @Override
    public BindingStatusResponse getBindingStatus(String uid, String userIp, String purchaseToken) {
        HttpHeaders headers = getHeaders(uid, userIp);

        ResponseEntity<BindingStatusResponse> response =
                restTemplate.exchange("/v2/bindings/{purchase_token}",
                        HttpMethod.GET,
                        new HttpEntity<>(headers),
                        BindingStatusResponse.class,
                        purchaseToken);
        return response.getBody();
    }

    @Override
    public void createProduct(CreateProductRequest createProductRequest) {
        HttpHeaders headers = getHeaders(null, null);

        headers.add("Content-Type", "application/json");

        ResponseEntity<String> entity = restTemplate.postForEntity("/v2/products",
                new HttpEntity<>(createProductRequest, headers),
                String.class
        );
    }

    @Override
    public SubscriptionShortInfo getSubscriptionShortInfo(String uid, String userIp, String subscriptionId) {
        HttpHeaders headers = getHeaders(uid, userIp);

        ResponseEntity<SubscriptionShortInfo> entity2 = restTemplate.exchange("/v2/subscriptions/{subscriptionId}",
                HttpMethod.GET,
                new HttpEntity<>(null, headers),
                SubscriptionShortInfo.class,
                subscriptionId
        );

        //TODO: обработка кода ошибки от trust-а

        return entity2.getBody();
    }

    @Override
    public List<String> createOrdersBatch(String uid, String userIp, List<String> productIds) {
        HttpHeaders headers = getHeaders(uid, userIp);
        headers.add("Content-Type", "application/json");

        ResponseEntity<CreateOrderBatchResponse> responseEntity = restTemplate.postForEntity("/v2/orders_batch",
                new HttpEntity<>(OrdersBatchRequest.create(productIds), headers),
                CreateOrderBatchResponse.class);

        if (responseEntity.getStatusCode() == HttpStatus.OK && "success".equals(responseEntity.getBody().getStatus())) {
            return responseEntity.getBody().getOrders()
                    .stream()
                    .map(CreateOrderBatchResponse.Order::getOrderId)
                    .collect(Collectors.toList());
        } else {
            throw new ProcessingException("Error creating order for products " + productIds);
        }
    }

    @Override
    public String createOrder(String uid, String userIp, String productId) {
        HttpHeaders headers = getHeaders(uid, userIp);
        headers.add("Content-Type", "application/json");

        ResponseEntity<CreateOrderResponse> responseEntity = restTemplate.postForEntity("/v2/orders",
                new HttpEntity<>(new CreateOrderRequest(productId), headers),
                CreateOrderResponse.class);

        if (responseEntity.getStatusCode() == HttpStatus.OK && "success".equals(responseEntity.getBody().getStatus())) {
            return responseEntity.getBody().getOrderId();
        } else {
            throw new ProcessingException("Error creating order for product " + productId);
        }
    }

    @Override
    public CreateRefundResponse createRefund(String uid, String ip,
                                             SubscriptionPaymentRefundParams createRefundRequest) {
        HttpHeaders headers = getHeaders(uid, ip);
        headers.add("Content-Type", "application/json");

        ResponseEntity<CreateRefundResponse> entity = restTemplate.postForEntity("/v2/refunds",
                new HttpEntity<>(createRefundRequest, headers),
                CreateRefundResponse.class);
        return Objects.requireNonNull(entity.getBody());
    }

    @Override
    public TrustRefundInfo startRefund(String uid, String ip, String refundId) {
        HttpHeaders headers = getHeaders(uid, ip);

        ResponseEntity<TrustRefundInfo> entity = restTemplate.postForEntity("/v2/refunds/{refundId}/start",
                new HttpEntity<>(headers),
                TrustRefundInfo.class,
                refundId);
        return Objects.requireNonNull(entity.getBody());
    }

    @Override
    public TrustRefundInfo getRefundStatus(String refundId) {
        return restTemplate.getForObject("/v2/refunds/{refundId}",
                TrustRefundInfo.class,
                refundId
        );
    }

    private HttpHeaders getHeaders(@Nullable String uid, @Nullable String userIp) {

        HttpHeaders headers = new HttpHeaders();
        if (useTvm) {
            headers.add(SERVICE_TICKET_HEADER, tvmClient.getServiceTicketFor(tvmClientId));
        }
        headers.add("X-Service-Token", serviceToken);

        if (uid != null) {
            headers.add("X-Uid", uid);
        }
        if (userIp != null) {
            headers.add("X-User-Ip", userIp);
        }

        return headers;
    }

    private record CardInfoWrapper(
            @JsonProperty("bound_payment_methods") List<PaymentMethod> boundPaymentMethods,
            @JsonProperty("status") String status
    ) {

        @JsonCreator
        CardInfoWrapper(
                @JsonProperty("bound_payment_methods") List<PaymentMethod> boundPaymentMethods,
                @JsonProperty("status") String status
        ) {
            this.boundPaymentMethods = boundPaymentMethods != null ? boundPaymentMethods : Collections.emptyList();
            this.status = status;
        }
    }
}
