package ru.yandex.alice.paskills.my_alice.layout;

import java.util.List;

import lombok.Data;

@Data
public class CarouselGroup implements PageLayout.Group {
    private final String type = "Carousel";
    private final String title;
    private final List<PageLayout.Card> items;
}
