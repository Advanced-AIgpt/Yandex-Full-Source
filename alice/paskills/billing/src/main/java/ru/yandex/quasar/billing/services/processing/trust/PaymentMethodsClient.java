package ru.yandex.quasar.billing.services.processing.trust;

import java.util.List;

import ru.yandex.quasar.billing.services.processing.TrustCurrency;

public interface PaymentMethodsClient {
    /**
     * Get lists of user payment methods
     *
     * @param uid    user id
     * @param userIp user IP
     * @return list of payment methods
     */
    List<PaymentMethod> getCardsList(String uid, String userIp);

    /**
     * create new binding session for a given currency and defined callback url
     *  @param uid      user identifier
     * @param userIp   user IP
     * @param currency binding currency
     * @param backUrl  binding redirect url
     * @param template
     */
    String createBinding(String uid, String userIp, TrustCurrency currency, String backUrl, TemplateTag template);

    /**
     * Start binding session for a given binging purchase token retrieved from
     * {@link #createBinding(String, String, TrustCurrency, String, TemplateTag)}
     *
     * @param uid           user identifier
     * @param userIp        user IP
     * @param purchaseToken purchase token
     * @return binging web view URL
     */
    String startBinding(String uid, String userIp, String purchaseToken);

    /**
     * get current status of the
     *
     * @param uid           user identifier
     * @param userIp        user IP
     * @param purchaseToken purchase token
     * @return detailed binding status
     */
    BindingStatusResponse getBindingStatus(String uid, String userIp, String purchaseToken);
}
