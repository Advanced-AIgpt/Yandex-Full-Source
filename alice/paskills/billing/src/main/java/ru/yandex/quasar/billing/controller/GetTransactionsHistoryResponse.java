package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.List;

import lombok.Data;

@Data
class GetTransactionsHistoryResponse {
    private final List<Item> items;

    @Data
    static class Item {
        private final String provider;
        private final String title;
        // private final PricingOption pricingOption;
        private final String maskedCardNumber;
        private final String paymentSystem;
        private final Instant purchaseDate;
        private final BigDecimal amount;
        private final String currency;
    }
}
