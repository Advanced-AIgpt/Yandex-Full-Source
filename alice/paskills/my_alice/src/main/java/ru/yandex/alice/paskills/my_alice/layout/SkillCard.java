package ru.yandex.alice.paskills.my_alice.layout;

import java.util.Map;

import lombok.Data;
import org.springframework.lang.Nullable;

@Data
public class SkillCard implements PageLayout.Card {
    private final String type = "SkillCard";
    private final String voiceSuggest;
    private final String text;
    private final String badgeType;
    private final String iconUrl;
    @Nullable
    private final Map<String, String> metrikaParams = null;
}
