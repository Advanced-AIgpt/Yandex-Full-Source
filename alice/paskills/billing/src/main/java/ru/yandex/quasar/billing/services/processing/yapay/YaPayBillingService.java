package ru.yandex.quasar.billing.services.processing.yapay;

import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.Data;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.SubscriptionShortInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;

public class YaPayBillingService implements ProcessingService, YandexPayMerchantRegistry {
    public static final String SKILL_PROVIDER_NAME = "skill";

    private final YandexPayClient yandexPayClient;
    private final TrustBillingService trustBillingService;
    private final BillingConfig config;
    private final AuthorizationContext authorizationContext;

    public YaPayBillingService(
            YandexPayClient yandexPayClient,
            TrustBillingService trustBillingService,
            BillingConfig config,
            AuthorizationContext authorizationContext) {
        this.yandexPayClient = yandexPayClient;
        this.trustBillingService = trustBillingService;
        this.config = config;
        this.authorizationContext = authorizationContext;
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createPayment(
            String uid, Long partnerId, String purchaseId, String securityToken,
            PricingOption selectedOption, String paymentCardId, String userIp, String userEmail,
            @Nullable Long merchantId
    ) {
        return createPayment(uid, null, null,
                selectedOption, paymentCardId, userEmail, merchantId);
    }

    @SuppressWarnings("ParameterNumber")
    public String createPayment(
            String uid,
            @Nullable SkillInfo skill,
            @Nullable PurchaseOffer purchaseOffer,
            PricingOption selectedOption,
            @Nullable String paymentCardId,
            String userEmail,
            Long merchantId
    ) {
        if (merchantId == null) {
            throw new YaPayClientException("merchant ID is empty");
        }

        var orderItems = selectedOption.getItems().stream()
                .map(item -> new OrderItem(item.getTitle(), TrustCurrency.RUB,
                        item.getNdsType(), item.getUserPrice(), item.getQuantity(), null))
                .collect(Collectors.toList());


        String purchaseOfferUrl = getPurchaseOfferUrl(skill, purchaseOffer,
                uid == null, purchaseOffer.getDeviceId(), true);

        CreateOrderRequest orderRequest = CreateOrderRequest.builder()
                .caption("Заказ " + purchaseOffer.getPurchaseRequestId())
                .description("Заказ из навыка Алисы")
                .userDescription("Заказ из навыка Алисы")
                .paymethodId(paymentCardId)
                .autoClear(false)
                .customerUid(uid)
                .items(orderItems)
                .returnUrl(purchaseOfferUrl)
                .userEmail(userEmail)
                .mode(purchaseOffer.isTestPayment() ? Mode.TEST : Mode.PROD)
                .build();
        Order order = yandexPayClient.createOrder(merchantId, orderRequest);
        return new IdTuple(merchantId, order.getOrderId()).purchaseToken();
    }

    @Override
    public Optional<String> startPayment(String uid, String userIp, String purchaseToken) {
        return startPayment(purchaseToken, null);
    }

    public Optional<String> startPayment(String purchaseToken, @Nullable PurchaseCompletionAction action) {
        IdTuple idTuple = IdTuple.parseToken(purchaseToken);
        var request = new StartOrderRequest(action, authorizationContext.getYandexUid());
        var startOrderResponse = yandexPayClient.startOrder(idTuple.merchantId, idTuple.orderId, request);
        return Optional.of(startOrderResponse.getTrustURl());
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createSubscription(
            String uid, Long partnerId, String subscriptionId, String securityToken,
            PricingOption selectedOption, int trialPeriodDays, String paymentCardId,
            String userIp, String userEmail, String provider
    ) {
        throw new YaPayClientException("subscriptions are not supported");
    }

    @Override
    public void startSubscription(String uid, String userIp, String purchaseToken) {
        throw new YaPayClientException("subscriptions are not supported");
    }

    @Override
    public PaymentInfo getPaymentShortInfo(String purchaseToken, String uid, String userIp) {
        IdTuple idTuple = IdTuple.parseToken(purchaseToken);
        Order order = yandexPayClient.getOrder(idTuple.merchantId, idTuple.orderId);
        PaymentInfo.Status status;
        switch (order.getPayStatus()) {
            case created:
                status = PaymentInfo.Status.AUTHORIZED;
                break;
            case held:
            case in_moderation:
                status = PaymentInfo.Status.AUTHORIZED;
                break;
            case in_progress:
            case paid:
                status = PaymentInfo.Status.CLEARED;
                break;
            case rejected:
            default:
                status = PaymentInfo.Status.ERROR_UNKNOWN;
                break;
        }

        return new PaymentInfo(
                purchaseToken,
                null,
                null,
                order.getPrice(),
                order.getCurrency().getCurrencyCode(),
                order.getCreated(),
                null,
                status,
                order.getClosed());
    }

    @Override
    public void clearPayment(String purchaseToken, String uid, String userIp) {
        IdTuple idTuple = IdTuple.parseToken(purchaseToken);
        // Я.Оплата не видит необходимости проверять uid покупателя в заказе и не видят в этом риска.
        yandexPayClient.clearOrder(idTuple.merchantId, idTuple.orderId);

    }

    @Override
    public void unholdPayment(String purchaseToken, String uid, String userIp) {
        IdTuple idTuple = IdTuple.parseToken(purchaseToken);
        // Я.Оплата не видит необходимости проверять uid покупателя в заказе и не видят в этом риска.
        yandexPayClient.unholdOrder(idTuple.merchantId, idTuple.orderId);
    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, String userIp) {
        return trustBillingService.getCardsList(uid, userIp);
    }

    @Override
    public SubscriptionShortInfo getSubscriptionShortInfo(String subscriptionId, String uid, String userIp) {
        throw new UnsupportedOperationException();
    }

    @Override
    public NewBindingInfo startBinding(String uid, String userIp, TrustCurrency currency, String returnUrl,
                                       TemplateTag template) {
        return trustBillingService.startBinding(uid, userIp, currency, returnUrl, template);
    }

    @Override
    public BindingInfo getBindingInfo(String uid, String userIp, String purchaseToken) {
        return trustBillingService.getBindingInfo(uid, userIp, purchaseToken);
    }

    @Override
    public ServiceMerchantInfo requestMerchantAccess(String token, String entityId, String description)
            throws TokenNotFound {
        try {
            return yandexPayClient.requestMerchantAccess(token, entityId, description);
        } catch (AccessRequestConflictException e) {
            return merchantInfo(e.getServiceMerchantId());
        }
    }

    @Override
    public ServiceMerchantInfo merchantInfo(long serviceMerchantId) {
        return yandexPayClient.merchantInfo(serviceMerchantId);
    }

    public String getPurchaseOfferUrl(SkillInfo skill, PurchaseOffer purchaseOffer, boolean anonymousRequest,
                                      String deviceId, boolean paymentConfirmedFlag) {
        UriComponentsBuilder builder =
                UriComponentsBuilder.fromUriString(config.getStorePurchasePath())
                        .queryParam("expectedUid", purchaseOffer.getUid())
                        .queryParam("purchaseOfferUuid", purchaseOffer.getUuid());

        builder.queryParam("skillId", skill.getSkillUuid());

        if (anonymousRequest) {
            builder.queryParam("bindFirst", true);
        }

        if (deviceId != null) {
            builder.queryParam("initial_device_id", deviceId);
        }
        if (paymentConfirmedFlag) {
            builder.queryParam("paymentConfirmed", true);
        }

        return builder.build().toUri().toASCIIString();
    }

    @Data
    private static class IdTuple {
        private final Long merchantId;
        private final Long orderId;

        static IdTuple parseToken(String token) {
            String[] split = token.split("\\|");
            String merchantId = split[0];
            String orderId = split[1];
            return new IdTuple(Long.valueOf(merchantId), Long.valueOf(orderId));
        }

        String purchaseToken() {
            return merchantId.toString() + "|" + orderId.toString();
        }
    }
}
