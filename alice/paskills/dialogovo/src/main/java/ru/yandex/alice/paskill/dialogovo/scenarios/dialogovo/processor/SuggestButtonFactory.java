package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor;

import java.util.Optional;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective;
import ru.yandex.alice.kronstadt.core.layout.Button;
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.paskill.dialogovo.scenarios.SemanticFrames;

@Component
public class SuggestButtonFactory {

    private final Phrases phrases;

    public SuggestButtonFactory(Phrases phrases) {
        this.phrases = phrases;
    }

    public Button getStopSuggest(String assistantName) {
        final String directiveText = String.format("%s, хватит.", assistantName);
        // на Станции этот саджест отображается как "Алиса, хватит" потому что Станция не следует протоколу спичкита
        // и берет любую директиву с payload: { "text": "" } в качестве текста саджеста
        return Button.withUrlAndPayload(
                "Закончить ❌",
                Optional.empty(), Optional.empty(), true, new TypeTextDirective(directiveText)
        );
    }

    public Button getGamesOnboaringSuggest() {
        return Button.simpleTextWithSemanticFrame(
                phrases.get("divs.games_onboarding.recommendation.message.suggest"),
                SemanticFrame.create(SemanticFrames.GAMES_ONBOARDING));
    }

}
