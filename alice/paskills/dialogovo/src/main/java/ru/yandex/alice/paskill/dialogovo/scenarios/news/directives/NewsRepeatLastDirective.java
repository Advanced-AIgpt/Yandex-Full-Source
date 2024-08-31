package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_repeat_last")
public class NewsRepeatLastDirective extends NewsCallbackDirective {
    @JsonCreator
    public NewsRepeatLastDirective(String skillId, String feedId, String contentId) {
        super(skillId, feedId, contentId);
    }
}
