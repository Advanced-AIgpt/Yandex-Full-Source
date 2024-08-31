package ru.yandex.alice.paskill.dialogovo.service.purchase;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;

public class InMemoryPurchaseCompleteResponseDaoImpl implements PurchaseCompleteResponseDao {
    private final Map<PurchaseCompleteResponseKey, WebhookResponse> responseState = new HashMap<>();

    @Override
    public Optional<WebhookResponse> findResponse(PurchaseCompleteResponseKey key) {
        return Optional.ofNullable(responseState.get(key));
    }

    @Override
    public void storeResponse(PurchaseCompleteResponseKey key, Instant timestamp, WebhookResponse response) {
        responseState.put(key, response);
    }

    @Override
    public void deleteResponse(PurchaseCompleteResponseKey key) {
        responseState.remove(key);
    }

    public void clear() {
        responseState.clear();
    }

    public List<PurchaseCompleteResponseStruct> getResponsesAsList() {
        return responseState.entrySet().stream()
                .map(e -> new PurchaseCompleteResponseStruct(e.getKey(), e.getValue()))
                .collect(Collectors.toList());
    }
}
