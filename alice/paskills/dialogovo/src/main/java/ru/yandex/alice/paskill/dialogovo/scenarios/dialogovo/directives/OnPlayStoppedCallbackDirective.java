package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.ToString;

import ru.yandex.alice.kronstadt.core.directive.Directive;

@Directive(value = "on_play_stopped", ignoreAnswer = true)
@ToString
public class OnPlayStoppedCallbackDirective extends AudioPlayerCallback {

    // left while callback refactoring otherwise tests brake
    @JsonProperty("@scenario_name")
    private final String scenarioName = "Dialogovo";

    @JsonCreator
    public OnPlayStoppedCallbackDirective(String skillId) {
        super(skillId);
    }
}
