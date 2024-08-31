package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Random;

import org.springframework.stereotype.Component;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody;
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.layout.Layout;
import ru.yandex.alice.kronstadt.core.layout.div.BackgroundType;
import ru.yandex.alice.kronstadt.core.layout.div.DivBackground;
import ru.yandex.alice.kronstadt.core.layout.div.DivBody;
import ru.yandex.alice.kronstadt.core.layout.div.DivState;
import ru.yandex.alice.kronstadt.core.layout.div.ImageElement;
import ru.yandex.alice.kronstadt.core.layout.div.Size;
import ru.yandex.alice.kronstadt.core.layout.div.TextStyle;
import ru.yandex.alice.kronstadt.core.layout.div.block.Block;
import ru.yandex.alice.kronstadt.core.layout.div.block.Position;
import ru.yandex.alice.kronstadt.core.layout.div.block.SeparatorBlock;
import ru.yandex.alice.kronstadt.core.layout.div.block.UniversalBlock;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.domain.ImageAlias;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingScenarioResponseFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.ArticleOverrides;
import ru.yandex.alice.paskill.dialogovo.utils.LogoUtils;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@Component
public class NewsFlashBriefingScenarioResponseFactory implements FlashBriefingScenarioResponseFactory {

    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final Phrases phrases;
    private final ArticleOverrides articleOverrides;
    private final Rate stalledContentActivationFailedRate;

    public NewsFlashBriefingScenarioResponseFactory(FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                                                    FlashBriefingSuggestsFactory suggestsFactory, Phrases phrases,
                                                    ArticleOverrides articleOverrides, MetricRegistry metricRegistry) {
        this.voiceButtonFactory = voiceButtonFactory;
        this.suggestsFactory = suggestsFactory;
        this.phrases = phrases;
        this.articleOverrides = articleOverrides;
        this.stalledContentActivationFailedRate =
                metricRegistry.rate("news.stalled.content.activation.failed.rate");
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onReadContent(DialogovoState dialogovoState,
                                                              Context context,
                                                              Optional<NewsSkillInfo> nextProviderPostrollO,
                                                              NewsArticle newsArticle,
                                                              MegaMindRequest<DialogovoState> request,
                                                              List<NewsSkillInfo> providersSuggest) {
        Nlg nlg = new ReadArticleNlg(newsArticle, phrases, voiceButtonFactory, suggestsFactory, nextProviderPostrollO,
                providersSuggest, articleOverrides, request, context)
                .renderArticle();

        if (request.getClientInfo().supportsDivCards()) {
            return getScenarioResponseBody(dialogovoState, context, nlg, generateArticleCard(newsArticle));
        } else {
            return getScenarioResponseBody(dialogovoState, context, nlg);
        }
    }

    @SuppressWarnings("ParameterNumber")
    public ScenarioResponseBody<DialogovoState> onReadContent(DialogovoState dialogovoState,
                                                              Context context,
                                                              Optional<NewsSkillInfo> nextProviderPostrollO,
                                                              NewsArticle newsArticle,
                                                              MegaMindRequest<DialogovoState> request,
                                                              List<NewsSkillInfo> providersSuggest,
                                                              boolean hasSubscription,
                                                              boolean allowSubscriptionSuggest) {
        return onReadContent(dialogovoState, context, nextProviderPostrollO, newsArticle, request, providersSuggest,
                hasSubscription, allowSubscriptionSuggest, false);
    }

    @SuppressWarnings("ParameterNumber")
    public ScenarioResponseBody<DialogovoState> onReadContent(DialogovoState dialogovoState,
                                                              Context context,
                                                              Optional<NewsSkillInfo> nextProviderPostrollO,
                                                              NewsArticle newsArticle,
                                                              MegaMindRequest<DialogovoState> request,
                                                              List<NewsSkillInfo> providersSuggest,
                                                              boolean hasSubscription,
                                                              boolean allowSubscriptionSuggest,
                                                              boolean skipIntro) {
        Nlg nlg = new ReadArticleNlg(newsArticle, phrases, voiceButtonFactory, suggestsFactory, nextProviderPostrollO,
                providersSuggest, articleOverrides, request, hasSubscription, allowSubscriptionSuggest, context,
                skipIntro)
                .renderArticle();

        boolean shouldListen = !skipIntro;
        if (request.getClientInfo().supportsDivCards()) {
            return getScenarioResponseBody(dialogovoState, context, nlg, generateArticleCard(newsArticle),
                    shouldListen);
        } else {
            return getScenarioResponseBody(dialogovoState, context, nlg, shouldListen);
        }
    }

    private DivBody generateArticleCard(NewsArticle newsArticle) {
        DivBody.DivBodyBuilder divBodyBuilder = DivBody.builder();

        divBodyBuilder.background(List.of(new DivBackground("#FFFFFF", BackgroundType.SOLID)));

        List<Block> blocks = new ArrayList<>();
        blocks.add(SeparatorBlock.withoutDelimiter(Size.XS));
        UniversalBlock.UniversalBlockBuilder mainBlockBuilder = UniversalBlock.builder();

        if (newsArticle.getSkill().getLogoUrl() != null) {
            mainBlockBuilder.sideElement(new UniversalBlock.SideElement(
                    new ImageElement(
                            LogoUtils.makeLogo(newsArticle.getSkill().getLogoUrl(), ImageAlias.MOBILE_LOGO_X2),
                            1),
                    Size.S,
                    Position.LEFT
            ));
        }

        mainBlockBuilder.text("<font color=\"#7F7F7F\">Новости</font>")
                .textStyle(TextStyle.TEXT_S)
                .title("<font color=\"#000000\">" + newsArticle.getSkill().getName() + "</font>")
                .titleStyle(TextStyle.TITLE_S);

        blocks.add(mainBlockBuilder.build());
        blocks.add(SeparatorBlock.withoutDelimiter(Size.XXS));

        divBodyBuilder.states(List.of(new DivState(1, blocks, null)));

        return divBodyBuilder.build();
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onGetDetails(DialogovoState dialogovoState,
                                                             Context context,
                                                             Random random,
                                                             NewsArticle newsArticle,
                                                             ClientInfo clientInfo) {
        Nlg nlg = new ArticleDetailsNlg(
                random, newsArticle, phrases, voiceButtonFactory, suggestsFactory, clientInfo)
                .render();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onSendDetailsLink(DialogovoState dialogovoState,
                                                                  Context context,
                                                                  Random random,
                                                                  NewsArticle newsArticle,
                                                                  ClientInfo clientInfo,
                                                                  List<ServerDirective> serverDirectives) {
        Nlg nlg = new SendArticleDetailsLinkNlg(
                random, newsArticle, phrases, voiceButtonFactory, suggestsFactory, clientInfo)
                .render();

        return getScenarioResponseBody(dialogovoState, context, nlg, serverDirectives);
    }

    @Override
    public ScenarioResponseBody<DialogovoState> onEmptyContentPerFeed(DialogovoState dialogovoState,
                                                                      Context context,
                                                                      Random random,
                                                                      NewsFeed newsFeed,
                                                                      NewsSkillInfo skillInfo,
                                                                      ClientInfo clientInfo,
                                                                      List<NewsSkillInfo> providersSuggest) {
        Nlg nlg = new NoNewsNlg(
                random, phrases, voiceButtonFactory, suggestsFactory, skillInfo.getId(), newsFeed.getId(),
                Optional.empty(), providersSuggest, clientInfo, context)
                .renderNoNextNews();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    @SuppressWarnings("ParameterNumber")
    public ScenarioResponseBody<DialogovoState> onNoNewFreshContentPerFeed(
            DialogovoState dialogovoState,
            Context context,
            Random random,
            NewsFeed newsFeed, NewsSkillInfo skillInfo,
            Optional<NewsSkillInfo> nextProviderPostrollO,
            ClientInfo clientInfo,
            List<NewsSkillInfo> providersSuggest
    ) {

        Nlg nlg = new NoNewsNlg(
                random, phrases, voiceButtonFactory, suggestsFactory, skillInfo.getId(), newsFeed.getId(),
                nextProviderPostrollO, providersSuggest, clientInfo, context)
                .renderNoFreshNews();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    @Override
    public ScenarioResponseBody<DialogovoState> repeatError(DialogovoState dialogovoState, Context context,
                                                            NewsFeed newsFeed,
                                                            NewsSkillInfo skillInfo, Random random) {

        Nlg nlg = new RepeatErrorNlg(phrases, suggestsFactory, voiceButtonFactory, skillInfo.getId(),
                newsFeed.getId(), random)
                .render();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    public ScenarioResponseBody<DialogovoState> onActivatingFailedRecommendNext(DialogovoState dialogovoState,
                                                                                Context context,
                                                                                Random random,
                                                                                NewsSkillInfo nextRecommended,
                                                                                ClientInfo clientInfo) {
        stalledContentActivationFailedRate.inc();
        Nlg nlg = new ActivateFailedRecommendNextNlg(
                phrases, random, nextRecommended, voiceButtonFactory, suggestsFactory, clientInfo)
                .render();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    public ScenarioResponseBody<DialogovoState> onRadionewsOnboardingSuggest(DialogovoState dialogovoState,
                                                                             Context context,
                                                                             Random random,
                                                                             List<NewsSkillInfo> suggested,
                                                                             ClientInfo clientInfo) {
        Nlg nlg = new RadionewsOnboardingNlg(
                phrases, random, suggested, voiceButtonFactory, suggestsFactory, clientInfo)
                .render();

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    public ScenarioResponseBody<DialogovoState> safeModeStub(DialogovoState dialogovoState, Context context,
                                                             MegaMindRequest<DialogovoState> request) {
        Nlg nlg = new Nlg(request.getRandom())
                .ttsWithText(phrases.getRandom("news.content.safe.mode.stub", request.getRandom()));

        return getScenarioResponseBody(dialogovoState, context, nlg);
    }

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(
            DialogovoState dialogovoState,
            Context context,
            Nlg nlg,
            List<ServerDirective> serverDirectives
    ) {
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

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(DialogovoState dialogovoState,
                                                                         Context context, Nlg nlg) {
        return getScenarioResponseBody(dialogovoState, context, nlg, true);
    }

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(DialogovoState dialogovoState, Context context,
                                                                         Nlg nlg, DivBody divBody) {
        return getScenarioResponseBody(dialogovoState, context, nlg, divBody, true);
    }

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(
            DialogovoState dialogovoState,
            Context context,
            Nlg nlg,
            boolean shouldListen
    ) {
        return new ScenarioResponseBody<>(
                Layout
                        .builder()
                        .outputSpeech(nlg.getTts().toString())
                        .textCard(nlg.getText().toString())
                        .shouldListen(shouldListen)
                        .suggests(nlg.getSuggests())
                        .build(),
                Optional.of(dialogovoState),
                context.getAnalytics().toAnalyticsInfo(),
                false,
                nlg.getActions());
    }

    private ScenarioResponseBody<DialogovoState> getScenarioResponseBody(DialogovoState dialogovoState, Context context,
                                                                         Nlg nlg, DivBody divBody,
                                                                         boolean shouldListen) {
        return new ScenarioResponseBody<>(
                Layout.cardBuilder(nlg.getTts().toString(), shouldListen, divBody)
                        .suggests(nlg.getSuggests())
                        .build(),
                Optional.of(dialogovoState),
                context.getAnalytics().toAnalyticsInfo(),
                false,
                nlg.getActions());
    }

}
