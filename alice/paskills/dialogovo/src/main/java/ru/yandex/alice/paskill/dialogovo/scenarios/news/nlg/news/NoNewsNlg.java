package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

import static ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory.createPostrollAction;

public class NoNewsNlg {

    private final Random random;
    private final Phrases phrases;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final String skillId;
    private final String feedId;
    private final Optional<NewsSkillInfo> nextProviderPostrollO;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final List<NewsSkillInfo> providersSuggest;
    private final ClientInfo clientInfo;
    private final Context context;

    @SuppressWarnings("ParameterNumber")
    public NoNewsNlg(Random random, Phrases phrases, FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                     FlashBriefingSuggestsFactory suggestsFactory, String skillId,
                     String feedId, Optional<NewsSkillInfo> nextProviderPostrollO,
                     List<NewsSkillInfo> providersSuggest, ClientInfo clientInfo, Context context) {
        this.random = random;
        this.phrases = phrases;
        this.voiceButtonFactory = voiceButtonFactory;
        this.skillId = skillId;
        this.feedId = feedId;
        this.nextProviderPostrollO = nextProviderPostrollO;
        this.suggestsFactory = suggestsFactory;
        this.providersSuggest = providersSuggest;
        this.clientInfo = clientInfo;
        this.context = context;
    }

    public Nlg renderNoNextNews() {
        return new Nlg(random)
                .ttsWithText(phrases.getRandom("news.content.empty.for.provider", random))
                .action("repeatAll", repeatAll())
                .action("repeatLast", repeatLast());
    }

    public Nlg renderNoFreshNews() {
        Nlg nlg = new Nlg(random);
        if (nextProviderPostrollO.isPresent()) {
            NewsSkillInfo nextProvidedPostroll = nextProviderPostrollO.get();
            context.getAnalytics().addAction(createPostrollAction(nextProvidedPostroll));
            nlg
                    .ttsWithText(phrases.getRandom("news.content.no.fresh.for.provider.suggest.another",
                            random, nextProvidedPostroll.getInflectedName()))
                    .action("activateNewsProviderWithConfirm",
                            activateNewsProviderWithConfirm(nextProvidedPostroll))
                    .action("activateNewsProviderByName",
                            activateNewsProviderByName(nextProvidedPostroll))
                    .action("activateDecline", activateDecline())
                    .suggest(suggestsFactory.provider(nextProvidedPostroll, clientInfo))
                    .suggest(suggestsFactory.repeatAllSuggest());

            if (clientInfo.supportsDivCards() || clientInfo.isNavigatorOrMaps()) {
                renderProvidersSuggests(nlg, providersSuggest
                        .stream()
                        // do not duplicate postroll provider
                        .filter(provider -> !provider.getId().equals(nextProvidedPostroll.getId()))
                        .collect(Collectors.toList())
                );
            }
        } else {
            nlg
                    .ttsWithText(phrases.getRandom("news.content.no.fresh.for.provider", random))
                    .action("repeatAllSuggestConfirm", repeatAllSuggestConfirm())
                    .suggest(suggestsFactory.repeatAllSuggest());

            if (clientInfo.supportsDivCards() || clientInfo.isNavigatorOrMaps()) {
                renderProvidersSuggests(nlg, providersSuggest);
            }
        }

        return nlg
                .action("repeatAll", repeatAll())
                .action("repeatLast", repeatLast());
    }

    protected void renderProvidersSuggests(Nlg nlg, List<NewsSkillInfo> suggests) {
        for (int i = 0; i < suggests.size(); i++) {
            NewsSkillInfo suggest = suggests.get(i);
            nlg.action("activateNewsProviderByName" + i, activateNewsProviderByName(suggest))
                    .suggest(suggestsFactory.provider(suggest, clientInfo));
        }
    }

    protected ActionRef activateNewsProviderByName(NewsSkillInfo skill) {
        return voiceButtonFactory.activateNewsProviderByName(skill, ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL);
    }

    private ActionRef repeatAll() {
        return voiceButtonFactory.repeatAll(skillId, feedId);
    }

    private ActionRef activateDecline() {
        return voiceButtonFactory.commonDeclineDoNothing();
    }

    private ActionRef activateNewsProviderWithConfirm(NewsSkillInfo skill) {
        return voiceButtonFactory.activateNewsProviderWithConfirm(skill,
                ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL);
    }

    private ActionRef repeatLast() {
        return voiceButtonFactory.repeatLastWithoutContent(skillId, feedId);
    }

    private ActionRef repeatAllSuggestConfirm() {
        return voiceButtonFactory.repeatAllSuggestConfirm(skillId, feedId);
    }
}
