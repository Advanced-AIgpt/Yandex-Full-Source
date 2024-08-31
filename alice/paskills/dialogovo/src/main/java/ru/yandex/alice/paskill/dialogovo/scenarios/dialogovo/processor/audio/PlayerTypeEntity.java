package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;


import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

@Getter
enum PlayerTypeEntity implements StringEnum {
    UNKNOWN("unknown"),
    VIDEO("video"),
    MUSIC("music"),
    RADIO("radio");

    public static final StringEnumResolver<PlayerTypeEntity> R = StringEnumResolver.resolver(PlayerTypeEntity.class);

    @Getter
    private final String value;

    PlayerTypeEntity(String value) {
        this.value = value;
    }

    @Override
    public String value() {
        return value;
    }
}
