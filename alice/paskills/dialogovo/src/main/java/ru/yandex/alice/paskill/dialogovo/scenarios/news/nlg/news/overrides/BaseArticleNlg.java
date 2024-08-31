package ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.news.overrides;

import java.util.List;
import java.util.Random;

import ru.yandex.alice.kronstadt.core.ActionRef;
import ru.yandex.alice.kronstadt.core.domain.ClientInfo;
import ru.yandex.alice.kronstadt.core.text.nlg.Nlg;
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsArticle;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsFeed;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingSuggestsFactory;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.nlg.FlashBriefingVoiceButtonsFactory;

public class BaseArticleNlg {

    private final FlashBriefingVoiceButtonsFactory voiceButtonFactory;
    private final FlashBriefingSuggestsFactory suggestsFactory;
    private final NewsArticle article;
    private final Random random;
    private final ClientInfo clientInfo;

    public BaseArticleNlg(FlashBriefingVoiceButtonsFactory voiceButtonFactory,
                          FlashBriefingSuggestsFactory suggestsFactory, NewsArticle article, Random random,
                          ClientInfo clientInfo) {
        this.voiceButtonFactory = voiceButtonFactory;
        this.suggestsFactory = suggestsFactory;
        this.article = article;
        this.random = random;
        this.clientInfo = clientInfo;
    }

    protected Nlg renderCommonControls(Nlg nlg) {
        if (article.getFeed().getDepth() > 1) {
            return nlg
                    .suggest(suggestsFactory.next(random))
                    .action("prev", prevAction())
                    .action("next", nextAction())
                    .action("repeatLast", repeatLastAction())
                    .action("repeatAll", repeatAllAction());
        } else {
            return nlg
                    .action("repeatLast", repeatLastAction())
                    .action("repeatAll", repeatAllAction());
        }
    }

    protected Nlg renderSendDetailsLinkControls(Nlg nlg) {
        if (article.getFeed().getDepth() > 1) {
            if (article.getContent().getDetailsUrl().isPresent()) {
                nlg
                        .suggest(suggestsFactory.sendDetailsLink(random));
            }
            return nlg.action("sendDetailsLink", getSendDetailsLink());
        } else {
            return nlg;
        }
    }

    protected Nlg renderDetailsControls(Nlg nlg) {
        if (article.getFeed().getDepth() > 1) {
            if (article.getContent().getDetailsUrl().isPresent() || article.getContent().getDetailsText().isPresent()) {
                nlg
                        .suggest(suggestsFactory.details(random));
            }
            return nlg
                    .action("details", getDetailsAction());
        } else {
            return nlg;
        }
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

    protected ActionRef subscriptionDecline(NewsSkillInfo skill, NewsFeed feed) {
        return voiceButtonFactory.subscriptionDecline(skill, feed);
    }

    protected ActionRef subscriptionConfirm(NewsSkillInfo skill, NewsFeed feed) {
        return voiceButtonFactory.subscriptionConfirm(skill, feed);
    }

    protected ActionRef getDetailsAction() {
        return voiceButtonFactory.details(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef getSendDetailsLink() {
        return voiceButtonFactory.sendDetailsLink(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef prevAction() {
        return voiceButtonFactory.prev(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef nextAction() {
        return voiceButtonFactory.next(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef repeatLastAction() {
        return voiceButtonFactory.repeatLast(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef activateNewsProviderWithConfirm(NewsSkillInfo skill) {
        return voiceButtonFactory.activateNewsProviderWithConfirm(skill,
                ActivationSourceType.RADIONEWS_INTERNAL_POSTROLL);
    }

    protected ActionRef activateDecline() {
        return voiceButtonFactory.commonDeclineDoNothing();
    }

    protected ActionRef getNextSuggestConfirm() {
        return voiceButtonFactory.nextSuggestConfirm(
                article.getSkill().getId(),
                article.getFeed().getId(),
                article.getContent().getId());
    }

    protected ActionRef repeatAllAction() {
        return voiceButtonFactory.repeatAll(
                article.getSkill().getId(),
                article.getFeed().getId());
    }
}
