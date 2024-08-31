package ru.yandex.alice.paskill.dialogovo.external.v1.response.audio;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;
import ru.yandex.alice.megamind.protos.scenarios.directive.TAudioPlayDirective.TBackgroundMode;

public enum BackgroundMode implements StringEnum {

    DUCKING("ducking", TBackgroundMode.Ducking),
    PAUSE("pause", TBackgroundMode.Pause);

    public static final StringEnumResolver<BackgroundMode> R =
            StringEnumResolver.resolver(BackgroundMode.class);

    @Getter
    @JsonValue
    private final String value;
    private final TBackgroundMode directiveMode;

    BackgroundMode(String value, TBackgroundMode directiveMode) {
        this.value = value;
        this.directiveMode = directiveMode;
    }

    @Override
    public String value() {
        return value;
    }

    public TBackgroundMode getDirectiveMode() {
        return directiveMode;
    }
}
