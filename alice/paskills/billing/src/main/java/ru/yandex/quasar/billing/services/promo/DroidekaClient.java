package ru.yandex.quasar.billing.services.promo;

import java.util.concurrent.CompletableFuture;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

public interface DroidekaClient {

    CompletableFuture<GiftState> getGiftStateAsync(String serial, String wifiMac, String ethernetMan);

    GiftState getGiftState(String serial, String wifiMac, String ethernetMan);

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    class GiftState {
        @JsonProperty("gift_available")
        private final boolean giftAvailable;

        public static final GiftState AVAILABLE = new GiftState(true);
        public static final GiftState NOT_AVAILABLE = new GiftState(false);

        @JsonCreator
        public static GiftState of(@JsonProperty("gift_available") boolean available) {
            return available ? AVAILABLE : NOT_AVAILABLE;
        }

    }
}
