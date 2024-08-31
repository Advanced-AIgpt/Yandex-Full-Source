package ru.yandex.alice.paskills.my_alice.layout;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Data;

@Data
public class ExpandableListGroup implements PageLayout.Group {

    private static final Integer DEFAULT_ROW_COUNT = 2;
    private final String type = "ExpandableList";
    private final String title;
    private final List<PageLayout.Card> items;
    @Nullable
    private final String moreButtonText;
    @Nullable
    private final String nextUrl;
    private final Integer initialRowsCount;


    private ExpandableListGroup(
            String title,
            List<PageLayout.Card> items,
            @Nullable String moreButtonText,
            @Nullable String nextUrl,
            Integer initialRowsCount
    ) {
        this.title = title;
        this.items = items;
        this.moreButtonText = moreButtonText;
        this.nextUrl = nextUrl;
        this.initialRowsCount = initialRowsCount;
    }

    public static ExpandableListGroup create(String title, List<PageLayout.Card> items) {
        return new ExpandableListGroup(title, items, null, null, DEFAULT_ROW_COUNT);
    }

    public static ExpandableListGroup create(
            String title,
            List<PageLayout.Card> items,
            String moreButtonText
    ) {
        return new ExpandableListGroup(
                title,
                items,
                moreButtonText,
                null,
                DEFAULT_ROW_COUNT);
    }

    public static ExpandableListGroup create(
            String title,
            List<PageLayout.Card> items,
            String moreButtonText,
            String nextUrl
    ) {
        return new ExpandableListGroup(
                title,
                items,
                moreButtonText,
                nextUrl,
                DEFAULT_ROW_COUNT);
    }

}
