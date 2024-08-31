package ru.yandex.alice.paskills.my_alice.layout;

import java.util.Map;

import lombok.Data;
import org.springframework.lang.Nullable;

@Data
public class AbilityCard implements PageLayout.Card {
    private final String type = "AbilityCard";
    private final String voiceSuggest;
    @Nullable
    private final Map<String, String> metrikaParams = null;
}
