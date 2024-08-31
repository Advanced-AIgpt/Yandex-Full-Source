package ru.yandex.quasar.billing.services.mediabilling;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import org.json.JSONObject;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.BindingStatusResponse;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethodsClient;
import ru.yandex.quasar.billing.services.processing.trust.ProcessingException;
import ru.yandex.quasar.billing.services.processing.trust.SubscriptionShortInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;

public class MediaBillingService implements ProcessingService {

    private final MediaBillingClient mediaBillingClient;
    private final PaymentMethodsClient cardsListClient;

    public MediaBillingService(
            MediaBillingClient mediaBillingClient,
            PaymentMethodsClient cardsListClient) {
        this.mediaBillingClient = mediaBillingClient;
        this.cardsListClient = cardsListClient;
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createPayment(String uid, Long partnerId, String purchaseId, String securityToken,
                                PricingOption selectedOption, String paymentCardId, String userIp, String userEmail,
                                @Nullable Long submerchantId) {
        throw new UnsupportedOperationException("direct purchases not allowed for MediaBilling");
    }

    @Override
    public Optional<String> startPayment(String uid, String userIp, String purchaseToken) {
        throw new UnsupportedOperationException("direct purchases not allowed for MediaBilling");
    }

    @SuppressWarnings("ParameterNumber")
    @Override
    public String createSubscription(String uid, Long partnerId, String subscriptionId, String securityToken,
                                     PricingOption selectedOption, int trialPeriodDays, String paymentCardId,
                                     String userIp, String userEmail, String provider) {
        try {
            String productId = new JSONObject(selectedOption.getProviderPayload()).optString("billing_product_id");

            SubmitNativeOrderResult result = mediaBillingClient.submitNativeOrder(uid, productId, paymentCardId);
            if (result.getStatus() == SubmitNativeOrderResult.Status.SUCCESS) {
                return result.getOrderId().toString();
            } else {
                throw new ProcessingException("Wrong mediabilling status: " + result.getStatus().name());
            }
        } catch (MediaBillingException e) {
            throw new ProcessingException(e);
        }
    }

    @Override
    public void startSubscription(String uid, String userIp, String purchaseToken) {
        // no implementation as subscription is started on creation
    }

    @Override
    public PaymentInfo getPaymentShortInfo(String orderId, String uid, String userIp) {
        OrderInfo orderInfo = mediaBillingClient.getOrderInfo(uid, orderId);
        PaymentInfo.Status status;
        switch (orderInfo.getStatus()) {
            case OK:
                status = PaymentInfo.Status.CLEARED;
                break;
            case REFUND:
                status = PaymentInfo.Status.REFUND;
                break;
            case CANCELLED:
                status = PaymentInfo.Status.ERROR_DO_NOT_TRY_LATER;
                break;
            case PENDING:
                status = PaymentInfo.Status.AUTHORIZED;
                break;
            case ERROR:
            default:
                status = PaymentInfo.Status.ERROR_UNKNOWN;
                break;
        }

        return new PaymentInfo(
                orderId,
                null,
                null,
                orderInfo.getPaidAmount(),
                orderInfo.getCurrency(),
                orderInfo.getCreated(),
                null,
                status,
                // treat mediabilling payments as cleared
                orderInfo.getCreated()
        );

    }

    @Override
    public void clearPayment(String purchaseToken, String uid, String userIp) {

    }

    @Override
    public void unholdPayment(String purchaseToken, String uid, String userIp) {

    }

    @Override
    public List<PaymentMethod> getCardsList(String uid, String userIp) {
        return cardsListClient.getCardsList(uid, userIp);
    }

    @Override
    public SubscriptionShortInfo getSubscriptionShortInfo(String subscriptionId, String uid, String userIp) {
        throw new UnsupportedOperationException();
    }

    @Override
    public NewBindingInfo startBinding(String uid, String userIp, TrustCurrency currency, String returnUrl,
                                       TemplateTag template) {
        BindCardResult result = mediaBillingClient.bindCard(uid, userIp, returnUrl, template);
        return new NewBindingInfo(result.bindingUrl(), result.purchaseToken());
    }

    @Override
    public BindingInfo getBindingInfo(String uid, String userIp, String purchaseToken) {
        BindingStatusResponse bindingStatus = cardsListClient.getBindingStatus(uid, userIp, purchaseToken);

        var status = BindingInfo.Status.forName(bindingStatus.getPaymentRespCode());

        return new BindingInfo(status, bindingStatus.getPaymentMethodId());
    }
}
