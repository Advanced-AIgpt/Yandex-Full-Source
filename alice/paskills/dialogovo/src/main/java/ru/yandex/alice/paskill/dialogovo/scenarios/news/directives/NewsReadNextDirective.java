package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_read_next")
public class NewsReadNextDirective extends NewsCallbackDirective {

    @JsonCreator
    public NewsReadNextDirective(String skillId, String feedId, String contentId) {
        super(skillId, feedId, contentId);
    }
}
