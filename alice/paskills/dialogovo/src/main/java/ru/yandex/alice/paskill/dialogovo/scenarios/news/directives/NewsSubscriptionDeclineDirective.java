package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_subscription_decline")
public class NewsSubscriptionDeclineDirective extends NewsFeedCallbackDirective {

    @JsonCreator
    public NewsSubscriptionDeclineDirective(String skillId, String feedId) {
        super(skillId, feedId);
    }
}
