package ru.yandex.alice.paskill.dialogovo.service.purchase;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;

public record PurchaseCompleteResponseStruct(PurchaseCompleteResponseKey key, WebhookResponse response) {
}
