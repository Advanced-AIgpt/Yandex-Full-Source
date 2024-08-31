package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.facts;

import java.util.List;
import java.util.Optional;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

import static java.util.Collections.emptyList;

@Component
public class FactsFlashBriefingScenarioResponseFactory implements FlashBriefingScenarioResponseFactory {

    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final Phrases phrases;

    public FactsFlashBriefingScenarioResponseFactory(FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                                                     FlashBriefingSuggestsFactory suggestsFactory, Phrases phrases) {
        this.voiceButtonFactory = voiceButtonFactory;
        this.suggestsFactory = suggestsFactory;
        this.phrases = phrases;
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onReadContent(DialogovoState dialogovoState,
                                                              Context context,
                                                              Optional<NewsSkillInfo> nextProviderPostrollO,
                                                              NewsArticle newsArticle,
                                                              MegaMindRequest<DialogovoState> request,
                                                              List<NewsSkillInfo> providersSuggest) {
        Nlg nlg = new ReadFactNlg(request.getRandom(), newsArticle, phrases, voiceButtonFactory, suggestsFactory)
                .renderArticle();

        return getScenarioResponseBody(dialogovoState, context, nlg, emptyList());
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onGetDetails(DialogovoState dialogovoState,
                                                             Context context,
                                                             Random random,
                                                             NewsArticle newsArticle,
                                                             ClientInfo clientInfo) {
        Nlg nlg = new ReadFactNlg(random, newsArticle, phrases, voiceButtonFactory, suggestsFactory)
                .renderDetails();

        return getScenarioResponseBody(dialogovoState, context, nlg, emptyList());
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onSendDetailsLink(DialogovoState dialogovoState,
                                                                  Context context,
                                                                  Random random,
                                                                  NewsArticle newsArticle,
                                                                  ClientInfo clientInfo,
                                                                  List<ServerDirective> serverDirectives) {
        Nlg nlg = new ReadFactNlg(random, newsArticle, phrases, voiceButtonFactory, suggestsFactory)
                .renderSendDetailsLink();

        return getScenarioResponseBody(dialogovoState, context, nlg, serverDirectives);
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onEmptyContentPerFeed(DialogovoState dialogovoState, Context context,
                                                                      Random random,
                                                                      NewsFeed newsFeed, NewsSkillInfo skillInfo,
                                                                      ClientInfo clientInfo,
                                                                      List<NewsSkillInfo> providersSuggest) {
        Nlg nlg = new NoFactsNlg(random, phrases, voiceButtonFactory, skillInfo.getId(), newsFeed.getId()).render();

        return getScenarioResponseBody(dialogovoState, context, nlg, emptyList());
    }

    public ScenarioResponseBody<DialogovoState> onNoNewFreshContentPerFeed(
            DialogovoState dialogovoState,
            Context context,
            Random random,
            NewsFeed newsFeed, NewsSkillInfo skillInfo,
            Optional<NewsSkillInfo> nextProviderPostrollO,
            ClientInfo clientInfo
    ) {
        return onEmptyContentPerFeed(dialogovoState, context, random, newsFeed, skillInfo, clientInfo, List.of());
    }

    @Override
    public ScenarioResponseBody<DialogovoState> repeatError(DialogovoState dialogovoState, Context context,
                                                            NewsFeed newsFeed,
                                                            NewsSkillInfo skillInfo, Random random) {

        Nlg nlg = new RepeatFactErrorNlg(phrases, suggestsFactory, voiceButtonFactory, skillInfo.getId(),
                newsFeed.getId(), random).render();

        return getScenarioResponseBody(dialogovoState, context, nlg, emptyList());
    }

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(
            DialogovoState dialogovoState,
            Context context,
            Nlg nlg,
            List<ServerDirective> serverDirectives) {
        return new ScenarioResponseBody<>(
                Layout.builder()
                        .outputSpeech(nlg.getTts().toString())
                        .textCard(nlg.getText().toString())
                        .shouldListen(true)
                        .suggests(nlg.getSuggests())
                        .build(),
                dialogovoState,
                context.getAnalytics().toAnalyticsInfo(),
                false,
                nlg.getActions(),
                null,
                serverDirectives);
    }
}
