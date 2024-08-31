package ru.yandex.quasar.billing.controller;

import lombok.Data;

import ru.yandex.quasar.billing.dao.PurchaseInfo;

@Data
class GetPurchaseStatusResponse {
    private final PurchaseInfo.Status status;
}
