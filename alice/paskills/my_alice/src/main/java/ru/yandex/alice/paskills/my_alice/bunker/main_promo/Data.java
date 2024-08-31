package ru.yandex.alice.paskills.my_alice.bunker.main_promo;

import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import com.google.gson.annotations.SerializedName;
import org.springframework.lang.Nullable;

@lombok.Data
class Data {
    @Nullable
    @SerializedName("show_probability")
    private final Double showProbability;
    @Nullable
    @SerializedName("items")
    private final List<Block> blocks;

    Double getShowProbability() {
        return Math.max(0.0, Math.min(1.0, Objects.requireNonNullElse(showProbability, 1.0)));
    }

    List<Block> getBlocks() {
        return Objects.requireNonNullElse(blocks,
                List.<Block>of())
                .stream()
                .filter(Objects::nonNull)
                .collect(Collectors.toList());
    }

    @lombok.Data
    static class Block {
        @Nullable
        private final String id;
        @Nullable
        private final Double weight;
        @Nullable
        private final String auth;
        @Nullable
        @SerializedName("items")
        private final List<Card> cards;

        Double getWeight() {
            return Math.max(0.0, Objects.requireNonNullElse(weight, 1.0));
        }

        String getAuth() {
            return Objects.requireNonNullElse(auth, "everybody");
        }

        List<Card> getCards() {
            return Objects.requireNonNullElse(cards,
                    List.<Card>of())
                    .stream()
                    .filter(Objects::nonNull)
                    .collect(Collectors.toList());
        }
    }

    @lombok.Data
    static class Card {
        @Nullable
        private final String id;
        private final Double weight;
        @Nullable
        private final String caption;
        @Nullable
        private final String text;
        @Nullable
        private final String button;
        @Nullable
        private final String image;
        @Nullable
        private final String color;

        Double getWeight() {
            return Math.max(0.0, Objects.requireNonNullElse(weight, 1.0));
        }
    }
}
