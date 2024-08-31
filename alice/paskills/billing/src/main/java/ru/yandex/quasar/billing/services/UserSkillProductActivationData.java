package ru.yandex.quasar.billing.services;

import lombok.Data;

@Data
public class UserSkillProductActivationData {
    private final String skillUuid;

    private final String skillName;

    private final String skillImageUrl;
}
