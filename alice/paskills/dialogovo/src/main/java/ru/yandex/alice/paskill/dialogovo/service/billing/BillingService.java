package ru.yandex.alice.paskill.dialogovo.service.billing;

import java.util.concurrent.CompletableFuture;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferPaymentInfoResponse;
import ru.yandex.alice.paskills.common.billing.model.api.PurchaseOfferStatusResponse;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductActivationResult;
import ru.yandex.alice.paskills.common.billing.model.api.UserSkillProductsResult;

/**
 * Service for working with billing component https://a.yandex-team.ru/arc/trunk/arcadia/alice/paskills/billing.
 */
public interface BillingService {
    CreatedPurchaseOffer createSkillPurchaseOffer(PurchaseOfferRequest request) throws BillingServiceException;

    CompletableFuture<UserSkillProductsResult> getUserSkillProductsAsync(String skillId);

    UserSkillProductsResult getUserSkillProducts(String skillId) throws BillingServiceException;

    UserSkillProductActivationResult activateUserSkillProduct(SkillInfo skillInfo, String tokenCode)
            throws BillingServiceException;

    PurchaseOfferStatusResponse getPurchaseStatus(String purchaseOfferUuid) throws BillingServiceException;

    PurchaseOfferPaymentInfoResponse getPurchaseOfferPaymentInfo(String purchaseOfferUuid)
            throws BillingServiceException;
}
