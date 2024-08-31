package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news;

import java.util.Random;

import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides.BaseArticleNlg;

public class SendArticleDetailsLinkNlg extends BaseArticleNlg {

    private final Random random;
    private final NewsArticle article;
    private final Phrases phrases;

    public SendArticleDetailsLinkNlg(Random random, NewsArticle article, Phrases phrases,
                                     FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                                     FlashBriefingSuggestsFactory suggestsFactory, ClientInfo clientInfo) {
        super(voiceButtonFactory, suggestsFactory, article, random, clientInfo);
        this.random = random;
        this.article = article;
        this.phrases = phrases;
    }

    public Nlg render() {
        Nlg nlg = new Nlg(random);
        renderCommonControls(nlg);

        if (article.getContent().getDetailsUrl().isEmpty()) {
            return nlg
                    .ttsWithText(phrases.getRandom("news.content.details.not.exists.text", this.random))
                    .action("nextConfirm", getNextSuggestConfirm());
        } else {
            return renderCommonControls(nlg)
                    .ttsWithText(phrases.getRandom("news.content.details.text", this.random));
        }
    }
}
