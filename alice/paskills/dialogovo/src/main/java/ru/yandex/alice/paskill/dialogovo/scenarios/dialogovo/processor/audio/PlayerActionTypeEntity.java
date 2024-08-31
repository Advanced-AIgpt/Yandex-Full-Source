package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;


import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

@Getter
enum PlayerActionTypeEntity implements StringEnum {
    UNKNOWN("unknown"),
    WATCH("watch"),
    LISTEN("listen"),
    PLAY("play");

    public static final StringEnumResolver<PlayerActionTypeEntity> R =
            StringEnumResolver.resolver(PlayerActionTypeEntity.class);

    @Getter
    private final String value;

    PlayerActionTypeEntity(String value) {
        this.value = value;
    }

    @Override
    public String value() {
        return value;
    }
}
