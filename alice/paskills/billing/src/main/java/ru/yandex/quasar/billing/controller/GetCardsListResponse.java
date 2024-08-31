package ru.yandex.quasar.billing.controller;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
class GetCardsListResponse {
    @JsonProperty("bound_payment_methods")
    private final List<CardInfo> cardList;

    @Data
    @Builder
    static class CardInfo {
        @JsonProperty("id")
        private final String id;
        @JsonProperty("payment_method")
        private final String paymentMethod;
        @JsonProperty("payment_system")
        private final String paymentSystem;
        private final String account;
        private final String system;
        private final Boolean expired;
    }
}
