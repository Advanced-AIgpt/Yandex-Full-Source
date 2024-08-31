package ru.yandex.alice.paskills.my_alice.layout;

import java.util.Map;

import lombok.Data;
import org.springframework.lang.Nullable;

@Data
public class PlusCard implements PageLayout.Card {
    private final String type = "PlusCard";
    private final String voiceSuggest;
    private final String titleText;
    private final String captionText;
    // TODO: add List<Item>
    // https://github.yandex-team.ru/search-interfaces/frontend/services/my-alice/src/models/plus-item.ts
    @Nullable
    private final Map<String, String> metrikaParams = null;
}
