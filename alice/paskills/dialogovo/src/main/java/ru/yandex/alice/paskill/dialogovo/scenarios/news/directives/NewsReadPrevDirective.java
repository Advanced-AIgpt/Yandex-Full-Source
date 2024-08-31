package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_read_prev")
public class NewsReadPrevDirective extends NewsCallbackDirective {

    @JsonCreator
    public NewsReadPrevDirective(String skillId, String feedId, String contentId) {
        super(skillId, feedId, contentId);
    }
}
