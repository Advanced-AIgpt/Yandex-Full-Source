package ru.yandex.quasar.billing.services;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nullable;

import lombok.Data;

import ru.yandex.alice.paskills.common.billing.model.api.SkillProductItem;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.exception.ProviderUnauthorizedException;
import ru.yandex.quasar.billing.exception.RetryPurchaseException;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.processing.trust.BindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.NewBindingInfo;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethod;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;

/**
 * Main entrypoint and manager of all purchase processing operations.
 * We store general processing information in tables. So we track subscriptions we create/renew/dismiss as an additional
 * source of information but go to providers for actual information about active subscriptions etc.
 * Stored in the DB data may be used to enrich responses with info if subscription is processed by us and though is
 * cancellable
 * ou our side.
 * <p>
 * Service allows processing and billing any kind of content either used by internal integrations or external one
 * <p>
 * BillingService is the only unit responsible for managing UserPurchases and UserSubscriptions tables
 *
 * <p>
 * Subscription can be:
 * CREATED - waiting for first payment to activate
 * ACTIVE - active subscriptions. subscriptions which we are going to renew in future.
 * DISMISSED - subscriptions with cancelled renewal
 * EXPIRED - subscriptions that were ACTIVE but was not renewed (not enough funds) and no expired.
 * <p>
 * Subscription's activeTill field is used to track active state of the subscription. It can be both dismissed
 * (cancelled)
 * but still be active until the end of its period after which it won't be renewed.
 */
public interface BillingService {

    String DELIVERY_PRODUCT_NAME = "Доставка";


    /**
     * Register internal callback for purchases.
     * If purchase is done through ContentService (for providers we natively integrate in scenarios) we process
     * purchase callback internally
     * otherwise provider's backend is called
     * Only one callback function per provider is allowed
     *
     * @param providerName     provider name
     * @param callbackFunction callback function to process purchase
     */
    void registerCallback(String providerName, ProviderOnPurchaseCallback callbackFunction)
            throws ProviderCallbackAlreadyRegisteredException;

    /**
     * Start purchase scenario for external skill. Create offer and send push to PP to accept the offer
     *
     * @param uid              user identifier
     * @param skill            skill requested purchase
     * @param skillOfferParams callback url
     * @param deviceId         device ID if started from speaker or other device that needs a delegated surface
     * @param purchaseRequest  purchase request parameters
     * @param sessionId        skill session ID
     * @param userId           skill user ID
     * @param webhookRequest   request related information
     * @return created skill purchase info
     */
    @SuppressWarnings("ParameterNumber")
    CreatedOffer createSkillPurchaseOffer(@Nullable String uid, SkillInfo skill, SkillOfferParams skillOfferParams,
                                          @Nullable String deviceId, SkillPurchaseOffer purchaseRequest,
                                          String sessionId, String userId, @Nullable String webhookRequest);

    /**
     * Get purchase offer data. Method is to be used by PP to render purchase page.
     *
     * @param uid               user identifier
     * @param purchaseOfferUuid UUID of the purchase offer
     * @return purchase offer data
     * @throws PurchaseOfferNotFoundException when purchase offer is not found for the user
     */
    PurchaseOffer getPurchaseOffer(String uid, String purchaseOfferUuid);

    /**
     * Get list of payments maid for a given purchase offer sorted chronologically
     *
     * @param uid               user identifier
     * @param purchaseOfferUuid UUID of the purchase offer
     * @return list of purchase offer payments
     */
    List<PaymentInfo> getPurchaseOfferPayments(String uid, String purchaseOfferUuid);

    /**
     * Start purchase transaction
     *
     * @param uid                 user identifier
     * @param selectedOption      selected pricing option
     * @param providerContentItem initially requested item. if user requested a film and we offed he subscription
     *                            purchase to acquire it
     * @param userIp              user IP
     * @param paymentCardId       card to pay from
     * @param userEmail           user email for reciept
     * @param trialPeriodDays     trial period
     * @return purchase token
     */
    @SuppressWarnings("ParameterNumber")
    String initPurchase(String uid, String provider, PricingOption selectedOption,
                        ProviderContentItem providerContentItem, String userIp, String paymentCardId,
                        String userEmail, @Nullable Integer trialPeriodDays);

    /**
     * Start purchase transaction for purchase offer
     *
     * @param uid                user identifier
     * @param purchaseOffer      UUID of the purchase offer
     * @param selectedOptionUuid selected pricing option UUID
     * @param skill              Skill info
     * @param userIp             user IP
     * @param paymentCardId      card to pay from
     * @param userEmail          user email for reciept
     * @param testPayment        is test transaction
     * @return purchase token
     * @throws PurchaseOfferNotFoundException     when purchase offer is not found for the user
     * @throws ProductNotFoundInPurchaseException when specified product is not listed in purchase offer's products
     */
    @SuppressWarnings("ParameterNumber")
    NewPurchase initPurchaseOffer(String uid, PurchaseOffer purchaseOffer, String selectedOptionUuid, SkillInfo skill,
                                  String userIp, @Nullable String paymentCardId, String userEmail, boolean testPayment);

    /**
     * @param purchaseId    purchase ID from callback parameters
     * @param securityToken security token from callback parameters
     * @param userIp        user IP for Trust
     * @return purchase result enum
     * @throws RetryPurchaseException                            if provider returned
     *                                                           {@link PurchaseInfo.Status#ERROR_TRY_LATER}
     * @throws UserPurchaseLockService.UserPurchaseLockException if multiple requests come on the same time
     */
    ProcessPurchaseResult processPurchaseCallback(String purchaseId, @Nullable String securityToken, String userIp)
            throws UserPurchaseLockService.UserPurchaseLockException;

    /**
     * @param purchaseToken purchase token from callback parameters
     * @param userIp        user IP for Trust
     * @return purchase result enum
     * @throws RetryPurchaseException                            if provider returned
     *                                                           {@link PurchaseInfo.Status#ERROR_TRY_LATER}
     * @throws UserPurchaseLockService.UserPurchaseLockException if multiple requests come on the same time
     */
    ProcessPurchaseResult processYaPayPurchaseCallback(String purchaseToken, String userIp)
            throws UserPurchaseLockService.UserPurchaseLockException;

    /**
     * Process refund callback, executed after {@code /unhold} call
     *
     * @param purchaseToken purchase token
     * @return processing result
     */
    ProcessRefundResult processRefundCallback(String purchaseToken);

    /**
     * Get subscription for purchase token
     *
     * @param purchaseToken purchase token
     * @return optional id subscription it corresponds to
     */
    Optional<Long> getSubscriptionForPurchase(String purchaseToken);


    /**
     * Create dummy PurchaseInfo record as currently WebView doesn't handle initPurchase request errors and only
     * handles by checking getPurchaseStatus
     *
     * @param uid                 user identifier
     * @param provider            provider name
     * @param status              dummy purchase status
     * @param providerContentItem item user wants to get
     * @param selectedOption      pricing option user selected to purchase to get the item
     * @param partnerId           partner id
     * @param paymentProcessor    payment processor
     * @return dummy purchase token to use in getPurchaseStatus requests
     */
    String createExceptionalPurchase(String uid, String provider, PurchaseInfo.Status status,
                                     @Nullable ProviderContentItem providerContentItem, PricingOption selectedOption,
                                     Long partnerId, PaymentProcessor paymentProcessor);

    /**
     * Get user card list
     *
     * @param uid       user identifier
     * @param processor {@link PaymentProcessor} payment processor
     * @param userIp    user IP address for Trust
     * @return list of cards
     */
    List<PaymentMethod> getCardsList(String uid, @Nullable PaymentProcessor processor, String userIp);

    void bindPurchaseOfferToUser(String purchaseOfferUuid, String uid);

    /**
     * Start binding on defined provider
     *
     * @param uid       user identifier
     * @param userIp    user IP address
     * @param processor PaymentProcessor
     * @param currency  currency
     * @param returnUrl return url
     * @return created binding info
     */
    NewBindingInfo startBinding(String uid, String userIp, PaymentProcessor processor, TrustCurrency currency,
                                String returnUrl, TemplateTag template);

    /**
     * Get binging process info
     *
     * @param uid           user identifier
     * @param userIp        user IP address
     * @param processor     payment processor
     * @param purchaseToken purchase token
     * @return binding information
     */
    BindingInfo getBindingInfo(String uid, String userIp, PaymentProcessor processor, String purchaseToken);

    /**
     * Get user transaction history
     *
     * @param uid    user identifier
     * @param userIp user IP address
     */
    List<TransactionInfo> getTransactionHistory(String uid, String userIp);

    List<TransactionInfoV2> getTransactionHistoryV2(String uid, String userIp, long offset, long limit);

    Optional<TransactionInfoV2> getTransactionV2(String uid, String userIp, long purchaseId);

    List<SkillProductItem> getAllUserSkillProductItems(String uid, String skillUuid);

    UserSkillProductActivationResult activateUserSkillProduct(String uid, UserSkillProductActivationData activationData,
                                                              String tokenCode);

    void deleteUserSkillProduct(String uid, String skillUuid, UUID productUuid);

    enum ProcessPurchaseResult {
        OK,
        OK_TRIAL,
        ALREADY_PROCESSED,
        PAYMENT_ERROR,
        CANCELLED,
        REFUNDED
    }

    enum ProcessRefundResult {
        PURCHASE_REFUNDED,
        PURCHASE_NOT_FOUND
    }

    @FunctionalInterface
    interface ProviderOnPurchaseCallback {
        void run(String provider, String uid, ProviderContentItem contentItem, PricingOption selectedOption,
                 String orderId, String purchaseToken) throws ProviderPurchaseException, ProviderUnauthorizedException;
    }

    @Data
    class SkillOfferParams {
        private final String name;
        private final String imageUrl;
        private final String callbackUrl;
    }

    @Data
    class CreatedOffer {
        private final String uuid;
        private final String url;
    }

    @Data
    class NewPurchase {
        private final String purchaseToken;
        private final String redirectUrl;
    }
}
