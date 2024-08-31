package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.directives;

import lombok.Data;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;

@Data
public abstract class AudioPlayerCallback implements CallbackDirective {
    private final String skillId;
}
