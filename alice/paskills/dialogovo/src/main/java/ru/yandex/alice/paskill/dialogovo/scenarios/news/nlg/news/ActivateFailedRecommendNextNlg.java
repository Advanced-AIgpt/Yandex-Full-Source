package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;


import java.util.Random;

import lombok.AllArgsConstructor;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

@AllArgsConstructor
public class ActivateFailedRecommendNextNlg {

    private final Phrases phrases;
    private final Random random;
    private final NewsSkillInfo nextRecommended;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final ClientInfo clientInfo;

    public Nlg render() {
        return new Nlg(random)
                .ttsWithText(phrases.getRandom("news.content.smth.failed.recommend.another",
                        random, nextRecommended.getInflectedName()))
                .action("activateNewsProviderWithConfirm", activateNewsProviderWithConfirm(nextRecommended))
                .action("activateNewsProviderByName", activateNewsProviderByName(nextRecommended))
                .action("activateDecline", activateDecline())
                .suggest(suggestsFactory.provider(nextRecommended, clientInfo));
    }

    private ActionRef activateNewsProviderWithConfirm(NewsSkillInfo skill) {
        return voiceButtonFactory.activateNewsProviderWithConfirm(skill,
                ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL);
    }

    private ActionRef activateDecline() {
        return voiceButtonFactory.commonDeclineDoNothing();
    }

    private ActionRef activateNewsProviderByName(NewsSkillInfo skill) {
        return voiceButtonFactory.activateNewsProviderByName(skill, ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL);
    }

}
