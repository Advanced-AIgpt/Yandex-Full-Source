package ru.yandex.alice.paskill.dialogovo.service.billing;

import javax.annotation.Nullable;
import javax.validation.Valid;
import javax.validation.constraints.NotEmpty;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.StartPurchase;


@Data
public class PurchaseOfferRequest {
    @JsonProperty("skill")
    private final BillingSkillInfo skillInfo;

    @JsonProperty("session_id")
    @NotEmpty
    private final String sessionId;

    @JsonProperty("user_id")
    @NotEmpty
    private final String userId;

    @JsonProperty("device_id")
    private final String deviceId;

    /**
     * This class is part of the dialogovo external api, so it should not be changed in case of changing billing api.
     */
    @Valid
    @JsonProperty("purchase_request")
    private final StartPurchase purchaseBasket;

    @Nullable
    @JsonProperty("webhook_request")
    private final PurchaseWebhookRequest purchaseWebhookRequest;

    @Data
    public static class BillingSkillInfo {
        @JsonProperty("id")
        @NotEmpty
        private final String skillUuid;

        @Size(max = 2048)
        @NotEmpty
        private final String name;

        @JsonProperty("image_url")
        @Size(max = 4096)
        private final String imageUrl;

        @JsonProperty("callback_url")
        private final String callbackUrl;

        public static BillingSkillInfo from(SkillInfo info, String callbackUrl) {
            return new BillingSkillInfo(
                    info.getId(),
                    info.getName(),
                    info.getLogoUrl(),
                    callbackUrl
            );
        }
    }
}
