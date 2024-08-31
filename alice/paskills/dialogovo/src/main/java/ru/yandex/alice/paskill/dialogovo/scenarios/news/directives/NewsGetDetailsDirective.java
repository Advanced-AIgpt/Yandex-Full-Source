package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_get_details")
public class NewsGetDetailsDirective extends NewsCallbackDirective {

    @JsonCreator
    public NewsGetDetailsDirective(String skillId, String feedId, String contentId) {
        super(skillId, feedId, contentId);
    }
}
