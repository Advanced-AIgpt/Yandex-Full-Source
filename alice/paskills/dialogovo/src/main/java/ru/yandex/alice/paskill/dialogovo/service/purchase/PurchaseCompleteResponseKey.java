package ru.yandex.alice.paskill.dialogovo.service.purchase;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class PurchaseCompleteResponseKey {
    @JsonProperty("user_id")
    private final String userId;

    @JsonProperty("skill_id")
    private final String skillId;

    @JsonProperty("purchase_offer_uuid")
    private final String purchaseOfferUuid;
}

