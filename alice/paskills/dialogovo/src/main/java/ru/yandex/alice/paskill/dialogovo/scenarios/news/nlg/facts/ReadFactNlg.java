package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.facts;

import java.util.List;
import java.util.Optional;
import java.util.Random;

import lombok.AllArgsConstructor;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.layout.TextWithTts;
import ru.yandex.alice.kronstadt.core.text.Phrases;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;
import ru.yandex.alice.paskill.dialogovo.utils.TextUtils;

import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.capitalizeFirst;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithDot;
import static ru.yandex.alice.paskill.dialogovo.utils.TextUtils.endWithoutTerminal;

@AllArgsConstructor
public class ReadFactNlg {

    private final Random random;
    private final NewsArticle article;
    private final Phrases phrases;
    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;

    public Nlg renderArticle() {
        Nlg nlg = new Nlg(random);

        renderBeforeNews(nlg);
        renderArticle(nlg);
        renderAfterNews(nlg);

        return nlg;
    }

    public Nlg renderSendDetailsLink() {
        Nlg nlg = new Nlg(random);


        if (article.getContent().getDetailsUrl().isEmpty()) {
            return nlg
                    .ttsWithText(phrases.getRandom("facts.content.details.not.exists.text", this.random))
                    .action("nextConfirm", getNextSuggestConfirm());
        } else {
            return renderCommonControls(nlg)
                    .ttsWithText(phrases.getRandom("facts.content.details.text", this.random));
        }
    }

    public Nlg renderDetails() {
        Nlg nlg = new Nlg(random);
        renderCommonControls(nlg);

        if (article.getContent().getDetailsText().isEmpty() && article.getContent().getDetailsUrl().isEmpty()) {
            return nlg
                    .ttsWithText(phrases.getRandom("facts.content.details.not.exists.text", this.random))
                    .action("nextConfirm", getNextSuggestConfirm());
        }

        if (article.getContent().getDetailsText().isPresent()) {
            nlg
                    .ttsWithText(article.getContent().getDetailsText().get());
        }

        if (article.getContent().getDetailsUrl().isPresent()) {
            renderSendDetailsLinkControlsSuggest(nlg);
            String sendPushSuggest = phrases.getRandom("facts.content.details.send.push.suggest", this.random);

            if (article.getContent().getDetailsText().isPresent()) {
                return nlg.maybe(0.2)
                        .tts(sendPushSuggest)
                        .end();
            } else {
                return nlg
                        .tts(sendPushSuggest);
            }
        }

        return nlg;
    }

    private Nlg renderAfterNews(Nlg nlg) {
        if (!article.isHasNext()) {
            return nlg.ttsPause(500).tts(phrases.getRandom("facts.content.finished", random));
        } else {
            return nlg
                    .maybe(0.3)
                    .tts(phrases.getRandom("facts.content.after.next.suggest.promo.tts", random))
                    .end();
        }
    }

    private void renderArticle(Nlg nlg) {
        renderArticleTts(nlg);
        renderArticleText(nlg);
        renderCommonControls(nlg);
        renderDetailsControls(nlg);
        renderSendDetailsLinkControls(nlg);
    }

    private Nlg renderCommonControls(Nlg nlg) {
        return nlg
                .suggest(suggestsFactory.next(random))
                .action("prev", prevAction())
                .action("next", nextAction())
                .action("repeatLast", repeatLastAction())
                .action("repeatAll", repeatAllAction());
    }

    private Nlg renderSendDetailsLinkControls(Nlg nlg) {
        if (article.getContent().getDetailsUrl().isPresent()) {
            nlg
                    .suggest(suggestsFactory.sendDetailsLink(random));
        }
        return nlg.action("sendDetailsLink", getSendDetailsLink());
    }

    private Nlg renderDetailsControls(Nlg nlg) {
        if (article.getContent().getDetailsUrl().isPresent() || article.getContent().getDetailsText().isPresent()) {
            nlg
                    .suggest(suggestsFactory.details(random));
        }
        return nlg
                .action("details", getDetailsAction());
    }

    private Nlg renderSendDetailsLinkControlsSuggest(Nlg nlg) {
        return nlg
                .suggest(suggestsFactory.sendDetailsLink(random))
                .action("sendDetailsLinkSuggestConfirm", getSendDetailsLinkConfirmAction())
                .action("sendDetailsLink", getSendDetailsLink());
    }

    private ActionRef prevAction() {
        return voiceButtonFactory.prev(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef nextAction() {
        return voiceButtonFactory.next(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef repeatLastAction() {
        return voiceButtonFactory.repeatLast(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef getDetailsAction() {
        return voiceButtonFactory.details(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef getSendDetailsLinkConfirmAction() {
        return voiceButtonFactory.sendDetailsLinkConfirm(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef getSendDetailsLink() {
        return voiceButtonFactory.sendDetailsLink(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef getNextSuggestConfirm() {
        return voiceButtonFactory.nextSuggestConfirm(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    private ActionRef repeatAllAction() {
        return voiceButtonFactory.repeatAll(
                article.getSkill().getId(),
                article.getFeed().getId());
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
        String title = capitalizeFirst(endWithDot(article.getContent().getTitle()));
        Optional<String> mainTextO = article.getContent().getMainText()
                .map(TextUtils::capitalizeFirst)
                .map(TextUtils::endWithDot);
        return nlg
                .text(title).text("\n\n").text(mainTextO);
    }

    private Nlg renderBeforeNews(Nlg nlg) {
        if (article.isDirectActivation()) {
            return renderPreamble(nlg);
        } else {
            return nlg;
        }
    }

    private Nlg renderPreamble(Nlg nlg) {
        TextWithTts preamble = getPreamble();
        return nlg
                .tts(preamble.getTts())
                .maybe(0.2)
                .tts(phrases.getRandom("facts.content.before.suggest.promo.tts", random))
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
        return phrases.getRandomTextWithTts("facts.content.preamble.default", random,
                List.of(article.getSkill().getName()),
                List.of(article.getSkill().getNameTts()));
    }

    private TextWithTts getPreambleFromFeed(String preamble) {
        return new TextWithTts(capitalizeFirst(endWithoutTerminal(preamble)));
    }
}
