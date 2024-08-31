package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_subscription_confirm")
public class NewsSubscriptionConfirmDirective extends NewsFeedCallbackDirective {

    @JsonCreator
    public NewsSubscriptionConfirmDirective(String skillId, String feedId) {
        super(skillId, feedId);
    }
}
