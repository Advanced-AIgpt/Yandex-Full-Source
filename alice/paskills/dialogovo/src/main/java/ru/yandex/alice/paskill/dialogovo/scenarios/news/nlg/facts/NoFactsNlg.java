package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.facts;

import java.util.Random;

import lombok.AllArgsConstructor;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

@AllArgsConstructor
public class NoFactsNlg {

    private final Random random;
    private final Phrases phrases;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final String skillId;
    private final String feedId;

    public Nlg render() {
        return new Nlg(random)
                .ttsWithText(phrases.getRandom("facts.content.empty.for.provider", random))
                .action("repeatAll", repeatAll());
    }

    private ActionRef repeatAll() {
        return voiceButtonFactory.repeatAll(skillId, feedId);
    }
}
