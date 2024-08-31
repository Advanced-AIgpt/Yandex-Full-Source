package ru.yandex.quasar.billing.services.processing;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.SubscriptionShortInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;

public class FreeProcessingBillingService implements ProcessingService {
    private static final String DEFAULT_CURRENCY = "RUB";

    private final TrustBillingService trustBillingService;
    private final UserPurchasesDAO userPurchasesDAO;

    public FreeProcessingBillingService(TrustBillingService trustBillingService, UserPurchasesDAO userPurchasesDAO) {
        this.trustBillingService = trustBillingService;
        this.userPurchasesDAO = userPurchasesDAO;
    }

    @Override
    @SuppressWarnings("ParameterNumber")
    public String createPayment(String uid, Long partnerId, String purchaseId, String securityToken,
                                PricingOption selectedOption, String paymentCardId, String userIp,
                                String userEmail, @Nullable Long serviceMerchantId) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Optional<String> startPayment(String uid, String userIp, String purchaseToken) {
        throw new UnsupportedOperationException();
    }

    @Override
    @SuppressWarnings("ParameterNumber")
    public String createSubscription(String uid, Long partnerId, String subscriptionId, String securityToken,
                                     PricingOption selectedOption, int trialPeriodDays, String paymentCardId,
                                     String userIp, String userEmail, String provider) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void startSubscription(String uid, String userIp, String purchaseToken) {
        throw new UnsupportedOperationException();
    }

    @Override
    public PaymentInfo getPaymentShortInfo(String purchaseToken, String uid, String userIp) {
        Optional<PurchaseInfo> purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseToken);
        return new PaymentInfo(
                purchaseToken,
                null,
                null,
                purchaseInfo.map(PurchaseInfo::getUserPrice).orElse(BigDecimal.ZERO),
                purchaseInfo.map(PurchaseInfo::getCurrencyCode).orElse(DEFAULT_CURRENCY),
                purchaseInfo.map(PurchaseInfo::getPurchaseDate).orElse(Instant.now()),
                null,
                PaymentInfo.Status.CLEARED,
                null);
    }

    @Override
    public void clearPayment(String purchaseToken, String uid, String userIp) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void unholdPayment(String purchaseToken, String uid, String userIp) {
        throw new UnsupportedOperationException();
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
        throw new UnsupportedOperationException();
    }

    @Override
    public BindingInfo getBindingInfo(String uid, String userIp, String purchaseToken) {
        throw new UnsupportedOperationException();
    }
}
