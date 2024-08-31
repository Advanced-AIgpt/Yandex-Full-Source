package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.stream.Collectors;

import ru.yandex.alice.kronstadt.core.MegaMindRequest;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.domain.Experiments;
import ru.yandex.alice.paskill.dialogovo.megamind.domain.Context;
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.ArticleOverrides;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.BaseArticleNlg;
import ru.yandex.alice.paskill.dialogovo.utils.TextUtils;

import static ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics.AnalyticsInfoActionsFactory.createPostrollAction;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.capitalizeFirst;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithDot;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithoutTerminal;

public class ReadArticleNlg extends BaseArticleNlg {

    private final Random random;
    private final NewsArticle article;
    private final Phrases phrases;
    private final Optional<NewsSkillInfo> nextProviderPostrollO;
    // suggest for devices with screen
    private final List<NewsSkillInfo> providersSuggest;
    private final ClientInfo clientInfo;
    private final ArticleOverrides articleOverrides;
    private final MegaMindRequest<DialogovoState> request;
    private final boolean hasSubscription;
    private final boolean allowSubscriptionSuggest;
    private final Context context;
    private final boolean skipIntroAndEnding;

    @SuppressWarnings("ParameterNumber")
    public ReadArticleNlg(NewsArticle article, Phrases phrases,
                          FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                          FlashBriefingSuggestsFactory suggestsFactory,
                          Optional<NewsSkillInfo> nextProviderPostrollO, List<NewsSkillInfo> providersSuggest,
                          ArticleOverrides articleOverrides, MegaMindRequest<DialogovoState> request,
                          boolean hasSubscription, boolean allowSubscriptionSuggest, Context context,
                          boolean skipIntroAndEnding) {
        super(voiceButtonFactory, suggestsFactory, article, request.getRandom(), request.getClientInfo());
        this.random = request.getRandom();
        this.article = article;
        this.phrases = phrases;
        this.request = request;
        this.nextProviderPostrollO = nextProviderPostrollO;
        this.providersSuggest = providersSuggest;
        this.clientInfo = request.getClientInfo();
        this.articleOverrides = articleOverrides;
        this.hasSubscription = hasSubscription;
        this.allowSubscriptionSuggest = allowSubscriptionSuggest;
        this.context = context;
        this.skipIntroAndEnding = skipIntroAndEnding;
    }

    @SuppressWarnings("ParameterNumber")
    public ReadArticleNlg(NewsArticle article, Phrases phrases,
                          FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                          FlashBriefingSuggestsFactory suggestsFactory,
                          Optional<NewsSkillInfo> nextProviderPostrollO, List<NewsSkillInfo> providersSuggest,
                          ArticleOverrides articleOverrides, MegaMindRequest<DialogovoState> request, Context context) {
        super(voiceButtonFactory, suggestsFactory, article, request.getRandom(), request.getClientInfo());
        this.random = request.getRandom();
        this.article = article;
        this.phrases = phrases;
        this.request = request;
        this.nextProviderPostrollO = nextProviderPostrollO;
        this.providersSuggest = providersSuggest;
        this.clientInfo = request.getClientInfo();
        this.articleOverrides = articleOverrides;
        this.context = context;
        this.hasSubscription = false;
        this.allowSubscriptionSuggest = false;
        this.skipIntroAndEnding = false;
    }

    public Nlg renderArticle() {
        Nlg nlg = new Nlg(random);

        if (!skipIntroAndEnding) {
            renderBeforeNews(nlg);
        }
        renderArticle(nlg);
        if (!skipIntroAndEnding) {
            renderControls(nlg);
            renderAfterNews(nlg);
        }

        return nlg;
    }

    private Nlg renderAfterNews(Nlg nlg) {
        if (clientInfo.supportsDivCards() || clientInfo.isNavigatorOrMaps()) {
            renderScreenSuggests(nlg);
        }

        if (!article.isHasNext()) {
            return renderAfterArticleWithoutNext(nlg);
        } else {
            return renderAfterArticleWithNext(nlg);
        }
    }

    private Nlg renderAfterArticleWithNext(Nlg nlg) {
        return nlg
                .maybe(0.3)
                .tts(phrases.getRandom("news.content.after.next.suggest.promo.tts", random))
                .end();
    }

    private Nlg renderAfterArticleWithoutNext(Nlg nlg) {
        double randomValue = random.nextDouble();
        if (randomValue <= request.getExperimentWithValue(Experiments.RADIONEWS_POSTROLL_PERC_LIMIT, 1)) {
            renderPostroll(nlg, randomValue);
        }

        return nlg;
    }

    private Nlg renderPostroll(Nlg nlg, double randomValue) {
        nlg.ttsPause(300);
        double postrollSuggestPerc =
                request.getExperimentWithValue(Experiments.RADIONEWS_SUBSCRIPTION_SUGGEST_PERC_LIMIT, 0.5);
        if (nextProviderPostrollO.isPresent()) {
            NewsSkillInfo nextProvidedPostroll = nextProviderPostrollO.get();

            if (article.isActivatedBySubscription()) {
                nlg
                        .maybe(0.2)
                        .tts(phrases.getRandom("news.content.finished.suggest.another", random,
                                nextProvidedPostroll.getInflectedName()));
            } else {
                //fixme: add subscription suggest once refused case
                if (randomValue <= postrollSuggestPerc && canSuggestSubscription()) {
                    suggestSubscription(nlg);
                } else {
                    if (randomValue <= 0.3) {
                        context.getAnalytics().addAction(createPostrollAction(nextProvidedPostroll));
                        nlg
                                .maybe(0.2)
                                .tts(phrases.getRandom("news.content.finished.suggest.another.with.direct.launch.promo",
                                        random,
                                        article.getSkill().getInflectedName(), nextProvidedPostroll.getInflectedName()))
                                .or()
                                .tts(phrases.getRandom("news.content.finished.suggest.another", random,
                                        nextProvidedPostroll.getInflectedName()));
                        nlg
                                .action("activateNewsProviderWithConfirm",
                                        activateNewsProviderWithConfirm(nextProvidedPostroll))
                                .action("activateDecline",
                                        activateDecline());
                    }
                }
            }
        } else {
            if (article.isActivatedBySubscription()) {
                nlg.maybe(0.2)
                        .tts(phrases.getRandom("news.content.finished", random));
            } else {
                if (randomValue <= postrollSuggestPerc && canSuggestSubscription()) {
                    suggestSubscription(nlg);
                } else {
                    nlg.maybe(0.2)
                            .tts(phrases.getRandom("news.content.finished.promo.direct.launch", random,
                                    article.getSkill().getInflectedName()))
                            .or()
                            .tts(phrases.getRandom("news.content.finished", random));
                }
            }
        }

        return nlg;
    }

    private boolean canSuggestSubscription() {
        return allowSubscriptionSuggest && !hasSubscription;
    }

    private void suggestSubscription(Nlg nlg) {
        //suggest subscribe
        nlg
                .tts(phrases.getRandom("news.content.subscription.suggest.promo", random,
                        article.getSkill().getInflectedName()));
        nlg.action("subscriptionConfirm",
                subscriptionConfirm(article.getSkill(), article.getFeed()));
        nlg.action("subscriptionDecline",
                subscriptionDecline(article.getSkill(), article.getFeed()));
    }

    private Nlg renderScreenSuggests(Nlg nlg) {
        List<NewsSkillInfo> providersSuggestForRender = this.providersSuggest;

        if (nextProviderPostrollO.isPresent()) {
            // make nextProvidedPostroll first
            NewsSkillInfo nextProvidedPostroll = nextProviderPostrollO.get();
            providersSuggestForRender = new ArrayList<>();
            providersSuggestForRender.add(nextProvidedPostroll);
            providersSuggestForRender.addAll(this.providersSuggest
                    .stream()
                    .filter(provider -> !provider.getId().equals(nextProvidedPostroll.getId()))
                    .collect(Collectors.toList()));
        }

        renderProvidersSuggests(nlg, providersSuggestForRender);
        return nlg;
    }

    private void renderArticle(Nlg nlg) {
        renderArticleTts(nlg);
        renderArticleText(nlg);
    }

    private void renderControls(Nlg nlg) {
        renderCommonControls(nlg);
        renderDetailsControls(nlg);
        renderSendDetailsLinkControls(nlg);
    }

    private Nlg renderOpusArticle(Nlg nlg, String soundId) {
        return nlg.opus(article.getSkill().getId(), soundId);
    }

    private Nlg renderArticleTts(Nlg nlg) {
        if (article.getContent().getSoundId().isPresent()) {
            return renderOpusArticle(nlg, article.getContent().getSoundId().get());
        } else {
            return renderTtsArticleTts(nlg);
        }
    }

    private Nlg renderTtsArticleTts(Nlg nlg) {
        String title = capitalizeFirst(endWithoutTerminal(article.getContent().getTitle()));
        Optional<String> mainTextO = article.getContent().getMainText()
                .map(TextUtils::capitalizeFirst)
                .map(TextUtils::endWithDot);
        return nlg
                .tts(title).ttsPause(500).tts(mainTextO);
    }

    private Nlg renderArticleText(Nlg nlg) {
        String articleTitle =
                articleOverrides.getOverrideO(article)
                        .flatMap(override -> override.overrideTitle(article, clientInfo))
                        .orElseGet(() -> article.getContent().getTitle());

        Optional<String> articleMainTextO =
                articleOverrides.getOverrideO(article)
                        .flatMap(override -> override.overrideMainText(article, clientInfo))
                        .orElse(article.getContent().getMainText());

        String title = capitalizeFirst(endWithDot(articleTitle));
        Optional<String> mainTextO = articleMainTextO
                .map(TextUtils::capitalizeFirst)
                .map(TextUtils::endWithDot);

        nlg.text(title);

        if (mainTextO.isPresent()) {
            nlg.text("\n\n").text(mainTextO);
        }

        return nlg;
    }

    private Nlg renderBeforeNews(Nlg nlg) {
        if (article.isActivatedBySubscription()) {
            return renderBeforeActivateBySubscription(nlg);
        } else if (article.isDirectActivation() && article.getFeed().getDepth() > 1) {
            return renderPreambleWithNext(nlg);
        } else if (article.isDirectActivation()) {
            return renderBeforeDirectAction(nlg);
        }

        return nlg;
    }

    private Nlg renderBeforeDirectAction(Nlg nlg) {
        String text = phrases.getRandom("news.content.start.ok", this.random);
        nlg
                .tts(text)
                .ttsPause(300);

        if (clientInfo.supportsDivCards()) {
            nlg.text(text);
        }

        return nlg;
    }

    private Nlg renderBeforeActivateBySubscription(Nlg nlg) {
        return nlg
                .tts(phrases.getRandom("news.content.by.subscription.preamble", random,
                        article.getSkill().getInflectedName()))
                .ttsPause(100)
                .maybe(0.07)
                .tts(phrases.getRandom("news.content.by.subscription.change.subscription.promo", random))
                .ttsPause(100)
                .end();
    }

    private Nlg renderPreambleWithNext(Nlg nlg) {
        TextWithTts preamble = getPreamble();
        return nlg
                .tts(preamble.getTts())
                .maybe(0.2)
                .tts(phrases.getRandom("news.content.before.suggest.promo.tts", random))
                .end()
                .ttsPause(1000)
                .text(preamble.getText()).text(":\n\n");
    }

    private TextWithTts getPreamble() {
        return article.getFeed().getPreamble().isPresent()
                ? getPreambleFromFeed(article.getFeed().getPreamble().get())
                : getDefaultPreamble();
    }

    private TextWithTts getDefaultPreamble() {
        return phrases.getRandomTextWithTts("news.content.preamble.default", random,
                List.of(article.getSkill().getName()),
                List.of(article.getSkill().getNameTts()));
    }

    private TextWithTts getPreambleFromFeed(String preamble) {
        return new TextWithTts(capitalizeFirst(endWithoutTerminal(preamble)));
    }
}
