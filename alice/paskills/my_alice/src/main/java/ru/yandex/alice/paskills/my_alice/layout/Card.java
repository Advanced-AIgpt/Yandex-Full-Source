package ru.yandex.alice.paskills.my_alice.layout;

import java.util.Map;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class Card implements PageLayout.Card {
    private final String type = "Card";
    private final String kind;
    @Nullable
    private final String voiceSuggest;
    @Nullable
    private final String captionText;
    @Nullable
    private final String mainText;
    @Nullable
    private final String imageUrl;
    @Nullable
    private final String color;
    @Nullable
    private final Map<String, String> metrikaParams;

    public Card(
            Kind kind,
            @Nullable String voiceSuggest,
            @Nullable String captionText,
            @Nullable String mainText,
            @Nullable String imageUrl,
            @Nullable String color,
            @Nullable Map<String, String> metrikaParams
    ) {
        this.kind = kind.getValue();
        this.voiceSuggest = voiceSuggest;
        this.captionText = captionText;
        this.mainText = mainText;
        this.imageUrl = imageUrl;
        this.color = color;
        this.metrikaParams = metrikaParams;
    }

    public enum Kind {
        STATION("station"),
        STATION_MINI("station-mini"),
        MUSIC("music"),
        SCENARIO("scenario");

        private final String value;

        Kind(String value) {
            this.value = value;
        }

        public String getValue() {
            return value;
        }
    }
}
