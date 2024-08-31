package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import com.fasterxml.jackson.annotation.JsonCreator;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("news_send_details_link")
public class NewsSendDetailsLinkDirective extends NewsCallbackDirective {

    @JsonCreator
    public NewsSendDetailsLinkDirective(String skillId, String feedId, String contentId) {
        super(skillId, feedId, contentId);
    }
}
