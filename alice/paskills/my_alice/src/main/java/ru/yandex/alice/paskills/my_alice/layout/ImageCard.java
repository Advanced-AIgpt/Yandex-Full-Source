package ru.yandex.alice.paskills.my_alice.layout;

import java.util.Map;

import lombok.Data;
import org.springframework.lang.Nullable;

@Data
public class ImageCard implements PageLayout.Card {
    private final String type = "ImageCard";
    @Nullable
    private final String title;
    private final String voiceSuggest;
    private final String imageUrl;
    private final String backgroundColor;
    private final Shape shape;
    private final Size size;
    @Nullable
    private final Map<String, String> metrikaParams = null;

    public ImageCard(
            @Nullable String title,
            String voiceSuggest,
            String imageUrl,
            String backgroundColor,
            Shape shape,
            Size size) {
        this.title = title;
        this.voiceSuggest = voiceSuggest;
        this.imageUrl = imageUrl;
        this.backgroundColor = backgroundColor;
        this.shape = shape;
        this.size = size;
    }


    public enum Shape {
        SQUARE("SQUARE"),
        CIRCLE("CIRCLE");

        private final String value;

        Shape(final String value) {
            this.value = value;
        }


        @Override
        public String toString() {
            return value;
        }
    }

    public enum Size {
        S("S"),
        M("M"),
        L("L");

        private final String value;

        Size(final String value) {
            this.value = value;
        }


        @Override
        public String toString() {
            return value;
        }
    }
}
