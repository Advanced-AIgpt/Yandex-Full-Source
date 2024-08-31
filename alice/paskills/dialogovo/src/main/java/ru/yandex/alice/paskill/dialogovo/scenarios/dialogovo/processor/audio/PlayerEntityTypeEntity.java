package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.audio;


import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

@Getter
enum PlayerEntityTypeEntity implements StringEnum {
    UNKNOWN("unknown"),
    ALBUM("album"),
    ARTIST("artist"),
    PLAYLIST("playlist"),
    TRACK("track"),
    VIDEO("video"),
    MUSIC("music"),
    BOOK("book");

    public static final StringEnumResolver<PlayerEntityTypeEntity> R =
            StringEnumResolver.resolver(PlayerEntityTypeEntity.class);

    @Getter
    private final String value;

    PlayerEntityTypeEntity(String value) {
        this.value = value;
    }

    @Override
    public String value() {
        return value;
    }
}
