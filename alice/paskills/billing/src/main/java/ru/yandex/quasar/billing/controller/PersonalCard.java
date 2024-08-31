package ru.yandex.quasar.billing.controller;

import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.quasar.billing.services.OfferCardData;

@Data
public class PersonalCard {
    private final Card card;

    @JsonProperty("remove_existing_cards")
    private final boolean removeExistingCards;

    @Data
    public static class Card {
        @JsonProperty("card_id")
        private final String cardId;

        @JsonProperty("button_url")
        private final String buttonUrl;

        private final String text;

        @JsonProperty("date_from")
        private final long dateFrom;

        @JsonProperty("date_to")
        private final long dateTo;

        @JsonProperty("yandex.station_film")
        private final Map<String, Object> yandexStationFilm;
    }

    @Nullable
    public static PersonalCard convertToPersonalCard(@Nullable OfferCardData offerCardData) {
        return offerCardData == null ? null : new PersonalCard(
                new PersonalCard.Card(
                        offerCardData.getCardId(),
                        offerCardData.getButtonUrl(),
                        offerCardData.getText(),
                        offerCardData.getDateFrom(),
                        offerCardData.getDateTo(),
                        offerCardData.getParams()
                ),
                true
        );
    }
}
