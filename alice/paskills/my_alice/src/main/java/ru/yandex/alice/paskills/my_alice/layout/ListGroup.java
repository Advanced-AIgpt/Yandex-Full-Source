package ru.yandex.alice.paskills.my_alice.layout;

import java.util.List;

import lombok.Data;

@Data
public class ListGroup implements PageLayout.Group {
    private final String type = "List";
    private final String title;
    private final List<PageLayout.Card> items;
    private final Params params;

    @Data
    public static class Params {
        private final String moreButtonText;
    }
}
