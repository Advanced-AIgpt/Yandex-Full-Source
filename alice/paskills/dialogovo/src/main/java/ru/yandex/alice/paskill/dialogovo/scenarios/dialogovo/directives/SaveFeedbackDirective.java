package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;
import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark;

@Directive("external_skill__save_feedback")
@Data
public class SaveFeedbackDirective implements CallbackDirective {

    @JsonProperty("skill_id")
    private final String skillId;

    @JsonProperty("mark")
    private final FeedbackMark feedbackMark;

}
