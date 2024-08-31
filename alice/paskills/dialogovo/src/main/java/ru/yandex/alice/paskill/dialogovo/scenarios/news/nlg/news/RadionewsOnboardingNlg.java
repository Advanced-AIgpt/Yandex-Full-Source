package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.List;
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
public class RadionewsOnboardingNlg {
    //  Keep in sync with RadionewsOnboardingRunProcessor.SUGGEST_PROVIDERS_NUM
    private static final int MAX_PROVIDERS_SUGGEST = 3;

    private final Phrases phrases;
    private final Random random;
    private final List<NewsSkillInfo> suggested;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final ClientInfo clientInfo;

    public Nlg render() {
        List<NewsSkillInfo> suggestedLimited = suggested.subList(0, Math.min(suggested.size(), MAX_PROVIDERS_SUGGEST));

        // length defence provided by çaller
        NewsSkillInfo activateSuggest = suggested.get(suggested.size() - 1);

        Nlg nlg = new Nlg(random)
                .ttsWithText(getPhraseBySuggestSize(suggestedLimited))
                .action("activateNewsProviderWithConfirm", activateNewsProviderWithConfirm(activateSuggest))
                .action("decline", activateDecline());

        for (NewsSkillInfo suggestMore : suggestedLimited) {
            nlg
                    .action("activateNewsProviderByName" + suggestMore.getId(),
                            activateNewsProviderByName(suggestMore))
                    .suggest(suggestsFactory.provider(suggestMore, clientInfo));
        }

        return nlg;
    }

    private String getPhraseBySuggestSize(List<NewsSkillInfo> allSuggested) {
        if (allSuggested.size() == 1) {
            return phrases.getRandom("news.content.radionews.onboarding.suggest.one.provider", random,
                    allSuggested.get(0).getInflectedName());
        } else if (allSuggested.size() == 2) {
            return phrases.getRandom("news.content.radionews.onboarding.suggest.two.providers", random,
                    allSuggested.get(0).getInflectedName(), allSuggested.get(1).getInflectedName());
        } else if (allSuggested.size() == 3) {
            return phrases.getRandom("news.content.radionews.onboarding.suggest.three.providers", random,
                    allSuggested.get(0).getInflectedName(), allSuggested.get(1).getInflectedName(),
                    allSuggested.get(2).getInflectedName());
        } else {
            // fallback
            return phrases.getRandom("news.content.radionews.onboarding.suggest.one.provider", random,
                    allSuggested.get(0).getInflectedName());
        }
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
