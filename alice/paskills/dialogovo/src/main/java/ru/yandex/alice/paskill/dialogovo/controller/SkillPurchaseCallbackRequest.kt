package ru.yandex.alice.paskill.dialogovo.controller

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonRawValue
import com.fasterxml.jackson.databind.node.ObjectNode
import ru.yandex.alice.paskill.dialogovo.service.billing.PurchaseWebhookRequest

data class SkillPurchaseCallbackRequest(
    @JsonProperty("purchase_request_id") val purchaseRequestId: String,
    @JsonProperty("purchase_token") val purchaseToken: String,
    @JsonProperty("purchase_offer_uuid") val purchaseOfferUuid: String,
    @JsonProperty("purchase_timestamp") val purchaseTimestamp: Long,
    @JsonProperty("skill_id") val skillId: String,
    @JsonProperty("user_id") val userId: String,
    @JsonRawValue @JsonProperty("purchase_payload") val purchasePayload: ObjectNode,
    @JsonProperty("webhook_request") val purchaseWebhookRequest: PurchaseWebhookRequest,
    @JsonProperty("version") val version: String,
)
