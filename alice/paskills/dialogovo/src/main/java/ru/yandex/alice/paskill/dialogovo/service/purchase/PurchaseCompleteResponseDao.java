package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.time.Instant;
import java.util.Optional;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;

public interface PurchaseCompleteResponseDao {
    Optional<WebhookResponse> findResponse(PurchaseCompleteResponseKey key);

    void storeResponse(PurchaseCompleteResponseKey key, Instant timestamp, WebhookResponse response);

    void deleteResponse(PurchaseCompleteResponseKey key);
}
