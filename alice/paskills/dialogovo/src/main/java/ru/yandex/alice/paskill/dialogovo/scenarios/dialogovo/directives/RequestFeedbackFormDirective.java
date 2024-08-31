package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("external_skill__request_feedback_form")
@Data
public class RequestFeedbackFormDirective implements CallbackDirective {

    @JsonProperty("skill_id")
    private final String skillId;

    //@JsonProperty("button_show_timestamp")
    @JsonIgnore // while refactoring realized the field was defined but never expected
    private final Instant buttonShowTimestamp;

}
