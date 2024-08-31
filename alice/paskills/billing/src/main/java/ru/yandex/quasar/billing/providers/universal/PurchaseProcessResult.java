package ru.yandex.quasar.billing.providers.universal;

import lombok.Data;

@Data
class PurchaseProcessResult {
    private final PurchaseStatus status;
}
