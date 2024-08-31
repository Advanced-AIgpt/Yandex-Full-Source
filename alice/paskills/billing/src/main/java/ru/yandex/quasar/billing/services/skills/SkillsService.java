package ru.yandex.quasar.billing.services.skills;

import java.util.List;

import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.SkillInfo;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.services.SkillAlreadyExistsException;

/**
 * Service to manipulate external skills billing info
 */
public interface SkillsService {

    /**
     * Register new skill and generate private keys
     *
     * @param skillUuid skill ID
     * @return created Skill parameters
     * @throws SkillAlreadyExistsException if skill is already registered
     */
    SkillInfo registerSkill(String skillUuid, long ownerUid, String slug) throws SkillAccessViolationException;

    /**
     * Request access to merchant in YandexPay
     *
     * @param skillUuid   skill ID
     * @param token       merchant token
     * @param description access request descriptions
     * @return merchant request info
     */
    MerchantInfo requestMerchantAccess(String skillUuid, String token, String description)
            throws BadSkillAccessRequestTokenException;

    /**
     * Get merchant access state
     *
     * @param skillUuid skill ID
     * @param token     token
     * @return merchant request info
     */
    MerchantInfo merchantInfo(String skillUuid, String token);

    /**
     * Get all merchants with requested access
     *
     * @param skillUuid skill ID
     * @return lest of merchants request info
     */
    List<MerchantInfo> getMerchants(String skillUuid);

    /**
     * Get external skill billing info by ID
     *
     * @param id skill record ID
     * @return skill information
     */
    SkillInfo getSkillById(Long id);

    /**
     * Get external skill billing info by skill UUID
     *
     * @param skillUuid skill uuid from dialog.platform
     * @return skill information
     */
    SkillInfo getSkillByUuid(String skillUuid);

    /**
     * Create new private and public keys for a skill
     * TODO remove in PASKILLS-5195
     *
     * @param skillInfoId skill info
     * @return skill information with new generated keys
     */
    @Deprecated
    SkillInfo refreshSkillKeys(Long skillInfoId);

    /**
     * execute purchase callback to skill backend with purchase information
     *
     * @param purchaseOffer purchase offer
     * @param purchaseToken purchase token
     * @param purchasePayload       payload provided by skill on offer creation
     * @param webhookRequest       payload related to request
     * @param eventType     event type
     * @throws ProviderPurchaseException if the skill failed to respond
     */
    void executeSkillCallback(
            PurchaseOffer purchaseOffer,
            String purchasePayload,
            String purchaseToken,
            String webhookRequest,
            PurchaseEventType eventType
    ) throws ProviderPurchaseException;

    /**
     * Type of purchase event for skill callback
     */
    enum PurchaseEventType {
        PURCHASE, REFUND
    }

    class BadSkillAccessRequestTokenException extends Exception {
        public BadSkillAccessRequestTokenException() {
        }

        public BadSkillAccessRequestTokenException(Throwable cause) {
            super(cause);
        }
    }

    class SkillAccessViolationException extends Exception {
        public SkillAccessViolationException() {
        }

        public SkillAccessViolationException(Throwable cause) {
            super(cause);
        }
    }
}
