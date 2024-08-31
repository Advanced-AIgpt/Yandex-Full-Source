package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_repeat_all")
public class NewsRepeatAllDirective extends NewsFeedCallbackDirective {

    @JsonCreator
    public NewsRepeatAllDirective(String skillId, String feedId) {
        super(skillId, feedId);
    }
}
