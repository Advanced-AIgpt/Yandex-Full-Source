package ru.yandex.quasar.billing.services.sup;

import java.util.Map;

import com.fasterxml.jackson.annotation.JsonAnyGetter;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class SendCardRequest {
    private final String uid;
    private final Card card;

    public static SendCardRequest create(String uid, String header, String linkUrl,
                                         Map<String, Object> additionalParameters, int dateToOffset, String cardId) {
        CardData data = new CardData(linkUrl, header, additionalParameters);
        long currentTimeMillis = System.currentTimeMillis();
        Card card = new Card(
                new CardInner(cardId, currentTimeMillis / 1000, currentTimeMillis / 1000 + dateToOffset, data),
                new SendDate(currentTimeMillis)
        );

        return new SendCardRequest(uid, card);
    }

    String getCardId() {
        return card.card.cardId;
    }

    @Data
    private static class Card {
        private final CardInner card;
        @JsonProperty("sent_date_time")
        private final SendDate sendDateTime;
    }

    @Data
    private static class CardInner {
        @JsonProperty("card_id")
        private final String cardId;
        private final String type = "yandex.station_film";
        @JsonProperty("date_from")
        private final long dateFrom;
        @JsonProperty("date_to")
        private final long dateTo;
        private final CardData data;
    }

    @Data
    private static class SendDate {
        // send date time in milliseconds
        @JsonProperty("$date")
        private final long date;
    }

    @Data
    private static class CardData {
        @JsonProperty("button_url")
        private final String buttonUrl;

        private final String text;

        private final Map<String, Object> additionalParameters;

        @JsonAnyGetter
        public Map<String, Object> getAdditionalParameters() {
            return additionalParameters;
        }
    }
}
