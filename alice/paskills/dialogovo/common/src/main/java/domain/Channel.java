package ru.yandex.alice.paskill.dialogovo.domain;

import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Getter;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;
import ru.yandex.alice.kronstadt.core.utils.StringEnumResolver;

public enum Channel implements StringEnum {
    UNKNOWN("unknown"),
    ALICE_SKILL("aliceSkill"),
    ORGANIZATION_CHAT("organizationChat"),
    SMART_HOME("smartHome"),
    THEREMINVOX("thereminvox"),
    ALICE_NEWS_SKILL("aliceNewsSkill");

    public static final StringEnumResolver<Channel> R =
            StringEnumResolver.resolver(Channel.class);

    @Getter
    @JsonValue
    private final String value;

    Channel(String value) {
        this.value = value;
    }

    @Override
    public String value() {
        return value;
    }
}
