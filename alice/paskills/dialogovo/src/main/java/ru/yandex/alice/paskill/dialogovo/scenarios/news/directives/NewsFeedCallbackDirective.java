package ru.yandex.alice.paskill.dialogovo.scenarios.news.directives;

import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;

@ToString
@EqualsAndHashCode
@Getter
@AllArgsConstructor
public abstract class NewsFeedCallbackDirective implements CallbackDirective {
    private final String skillId;
    private final String feedId;
}
