package ru.yandex.quasar.billing.services.sup;

import java.util.ArrayList;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class GetCardsResponse {
    private List<CardId> blocks = new ArrayList<>();

    List<CardId> getBlocks() {
        return blocks;
    }

    @Data
    static class CardId {
        @JsonProperty("card_id")
        private String cardId;

        String getCardId() {
            return cardId;
        }
    }
}
