package ru.yandex.alice.paskill.dialogovo.domain;

import javax.annotation.Nullable;

import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillDeveloperType;

public enum DeveloperType {
    Yandex("yandex"),
    External("external");

    private final String value;

    DeveloperType(String value) {
        this.value = value;
    }

    public static DeveloperType fromString(@Nullable String developerType) {
        if ("yandex".equals(developerType)) {
            return DeveloperType.Yandex;
        } else {
            return DeveloperType.External;
        }
    }

    public String getValue() {
        return value;
    }

    public static TSkillDeveloperType getProtoDeveloperType(DeveloperType source) {
        switch (source) {
            case Yandex:
                return TSkillDeveloperType.Yandex;
            default:
                return TSkillDeveloperType.External;
        }
    }
}
