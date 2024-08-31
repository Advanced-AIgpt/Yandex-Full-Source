package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.text.InflectedString;

import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.GENITIVE;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.NOMINATIVE;
import static ru.yandex.alice.kronstadt.core.text.InflectedString.RuCase.PREPOSITIONAL;

@Component
// TODO: move overrides to DB
public class NewsSkillInflectedNamesOverrides {

    private static final Map<String, InflectedString> SILVER_RAIN = Map.of(
            "6f852374-serebryanyj-dozh",
            InflectedString.cons(Map.of(
                    NOMINATIVE, "Серебряный дождь",
                    GENITIVE, "Серебряного дождя",
                    PREPOSITIONAL, "Серебряном Дожде")));

    private final Map<String, InflectedString> bySlugOverrides;

    public NewsSkillInflectedNamesOverrides() {
        Map<String, InflectedString> overrides = new HashMap<>();
        overrides.putAll(SILVER_RAIN);

        this.bySlugOverrides = Map.copyOf(overrides);
    }

    public Optional<InflectedString> getOverrideO(String slug) {
        return Optional.ofNullable(bySlugOverrides.get(slug));
    }
}
