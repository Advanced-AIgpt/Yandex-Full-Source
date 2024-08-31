package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.Random;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.BaseArticleNlg;

import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.capitalizeFirst;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithDot;

public class ArticleDetailsNlg extends BaseArticleNlg {

    private final Random random;
    private final NewsArticle article;
    private final Phrases phrases;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;

    public ArticleDetailsNlg(Random random, NewsArticle article, Phrases phrases,
                             FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                             FlashBriefingSuggestsFactory suggestsFactory, ClientInfo clientInfo) {
        super(voiceButtonFactory, suggestsFactory, article, random, clientInfo);
        this.random = random;
        this.article = article;
        this.phrases = phrases;
        this.voiceButtonFactory = voiceButtonFactory;
        this.suggestsFactory = suggestsFactory;
    }

    public Nlg render() {
        Nlg nlg = new Nlg(random);
        renderCommonControls(nlg);

        if (article.getContent().getDetailsText().isEmpty() && article.getContent().getDetailsUrl().isEmpty()) {
            return nlg
                    .ttsWithText(phrases.getRandom("news.content.details.not.exists.text", this.random))
                    .action("nextConfirm", getNextSuggestConfirm());
        }

        if (article.getContent().getDetailsText().isPresent()) {
            nlg
                    .ttsWithText(capitalizeFirst(endWithDot(article.getContent().getDetailsText().get())));
        }

        if (article.getContent().getDetailsUrl().isPresent()) {
            renderSendDetailsLinkControlsSuggest(nlg);
            String sendPushSuggest = phrases.getRandom("news.content.details.send.push.suggest", this.random);

            if (article.getContent().getDetailsText().isPresent()) {
                return nlg
                        .ttsPause(300)
                        .maybe(0.2)
                        .tts(sendPushSuggest)
                        .end();
            } else {
                return nlg
                        .tts(sendPushSuggest);
            }
        }

        return nlg;
    }

    private Nlg renderSendDetailsLinkControlsSuggest(Nlg nlg) {
        return nlg
                .suggest(suggestsFactory.sendDetailsLink(random))
                .action("sendDetailsLinkSuggestConfirm", getSendDetailsLinkConfirmAction())
                .action("sendDetailsLink", getSendDetailsLink());
    }

    private ActionRef getSendDetailsLinkConfirmAction() {
        return voiceButtonFactory.sendDetailsLinkConfirm(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }
}
