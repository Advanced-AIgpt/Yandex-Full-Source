package ru.yandex.alice.paskill.dialogovo.scenarios.news.domain;

import lombok.Data;

@Data
public class NewsArticle {

    private final NewsSkillInfo skill;
    private final NewsFeed feed;
    private final NewsContent content;
    private final boolean directActivation;
    private final boolean hasNext;
    private final boolean activatedBySubscription;

    public NewsArticle(NewsSkillInfo skill, NewsFeed feed, NewsContent content, boolean directActivation,
                       boolean hasNext) {
        this.skill = skill;
        this.feed = feed;
        this.content = content;
        this.directActivation = directActivation;
        this.hasNext = hasNext;
        this.activatedBySubscription = false;
    }

    public NewsArticle(NewsSkillInfo skill, NewsFeed feed, NewsContent content, boolean directActivation,
                       boolean hasNext,
                       boolean activatedBySubscription) {
        this.skill = skill;
        this.feed = feed;
        this.content = content;
        this.directActivation = directActivation;
        this.hasNext = hasNext;
        this.activatedBySubscription = activatedBySubscription;
    }
}
