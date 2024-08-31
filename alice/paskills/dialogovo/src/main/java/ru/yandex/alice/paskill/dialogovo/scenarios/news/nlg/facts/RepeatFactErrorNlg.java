package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.facts;

import java.util.Random;

import lombok.AllArgsConstructor;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

@AllArgsConstructor
public class RepeatFactErrorNlg {

    private final Phrases phrases;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final String skillId;
    private final String feedId;
    private final Random random;

    public Nlg render() {
        return new Nlg(random)
                .ttsWithText(phrases.get("facts.content.repeat.all.suggest"))
                .action("repeatAllSuggest", repeatAllSuggestConfirm())
                .suggest(suggestsFactory.repeatAllSuggestDecline())
                .suggest(suggestsFactory.repeatAllSuggestConfirm());
    }

    private ActionRef repeatAllSuggestConfirm() {
        return voiceButtonFactory.repeatAllSuggestConfirm(skillId, feedId);
    }
}
