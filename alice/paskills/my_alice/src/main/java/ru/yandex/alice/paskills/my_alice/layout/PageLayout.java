package ru.yandex.alice.paskills.my_alice.layout;

import java.util.List;
import java.util.Map;

import lombok.Data;

@Data
public class PageLayout {
    private final List<Group> groups;

    public interface Group {
        String getType();

        String getTitle();

        List<Card> getItems();
    }

    public interface Card {
        String getType();

        String getVoiceSuggest();

        Map<String, String> getMetrikaParams();
    }
}

