package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg;


import java.util.Map;
import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.FlashBriefingType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.facts.FactsFlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.NewsFlashBriefingScenarioResponseFactory;

@Component
public class ScenarioResponseResolver {

    private final Map<FlashBriefingType, FlashBriefingScenarioResponseFactory> typeToFactory;
    private final FlashBriefingScenarioResponseFactory defaultFactory;

    public ScenarioResponseResolver(NewsFlashBriefingScenarioResponseFactory newsFactory,
                                    FactsFlashBriefingScenarioResponseFactory factsFactory) {
        this.typeToFactory = Map.of(
                FlashBriefingType.RADIONEWS, newsFactory,
                FlashBriefingType.FACTS, factsFactory
        );
        this.defaultFactory = newsFactory;
    }

    public FlashBriefingScenarioResponseFactory get(FlashBriefingType flashBriefingType) {
        return Optional.ofNullable(typeToFactory.get(flashBriefingType)).orElse(defaultFactory);
    }
}
