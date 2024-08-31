package ru.yandex.quasar.billing.services.processing;

import java.util.List;
import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.SubscriptionShortInfo;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;

public interface ProcessingService {

    /**
     * Create new payment in processing system
     *
     * @param uid               user identifier
     * @param partnerId         partner who sells selected option
     * @param purchaseId        id of registered in billing purchase
     * @param securityToken     security token of registered in billing purchase
     * @param selectedOption    selected pricing option for purchase
     * @param paymentCardId     card ID
     * @param userIp            user IP
     * @param userEmail         user email for receipts
     * @param serviceMerchantId Yandex Pay service merchant ID
     * @return purchase token
     */
    @SuppressWarnings("ParameterNumber")
    String createPayment(String uid, Long partnerId, String purchaseId, String securityToken,
                         PricingOption selectedOption, String paymentCardId, String userIp, String userEmail,
                         @Nullable Long serviceMerchantId);

    Optional<String> startPayment(String uid, String userIp, String purchaseToken);

    /**
     * Create new subscription in processing system
     *
     * @param uid             user identifier
     * @param partnerId       partner who sells selected option
     * @param subscriptionId  id of registered in billing subscription
     * @param securityToken   security token of registered in billing purchase
     * @param selectedOption  selected pricing option for subscription
     * @param trialPeriodDays subscription trial period in days, 0 if not trial period defined
     * @param paymentCardId   card ID
     * @param userIp          user IP
     * @param userEmail       user email for receipts
     * @param provider        name of native the content provider selectedOption corresponds to
     * @return purchase token
     */
    @SuppressWarnings("ParameterNumber")
    String createSubscription(String uid, Long partnerId, String subscriptionId, String securityToken,
                              PricingOption selectedOption, int trialPeriodDays, String paymentCardId, String userIp,
                              String userEmail, String provider);

    /**
     * start processing of the given subscription
     *
     * @param uid           user identifier
     * @param userIp        user IP
     * @param purchaseToken purchase token
     */
    void startSubscription(String uid, String userIp, String purchaseToken);

    PaymentInfo getPaymentShortInfo(String purchaseToken, String uid, String userIp);

    void clearPayment(String purchaseToken, String uid, String userIp);

    void unholdPayment(String purchaseToken, String uid, String userIp);

    /**
     * get user cards from TRUST
     *
     * @param uid    user identifier
     * @param userIp user IP address
     * @return list of user payment methods
     */
    List<PaymentMethod> getCardsList(String uid, String userIp);

    SubscriptionShortInfo getSubscriptionShortInfo(String subscriptionId, String uid, String userIp);

    /**
     * Start card binding process
     *
     * @param uid       user ID
     * @param userIp    user IP
     * @param currency  binding currency
     * @param returnUrl return url
     * @param template  template for
     * @return binding info
     */
    NewBindingInfo startBinding(String uid, String userIp, TrustCurrency currency, String returnUrl,
                                TemplateTag template);


    /**
     * get binding info with binding status and added payment method
     *
     * @param uid           user ID
     * @param userIp        user IP
     * @param purchaseToken purchase token
     * @return binding info
     */
    BindingInfo getBindingInfo(String uid, String userIp, String purchaseToken);
}
