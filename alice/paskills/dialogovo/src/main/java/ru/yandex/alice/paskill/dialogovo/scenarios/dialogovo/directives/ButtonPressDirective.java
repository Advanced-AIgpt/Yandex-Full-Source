package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive("external_skill__on_external_button")
@Data
public class ButtonPressDirective implements CallbackDirective {

    private final String text;
    private final String payload;

    @JsonProperty("request_id")
    private final String requestId;

    public String getText() {
        return text;
    }

    public String getPayload() {
        return payload;
    }
}
