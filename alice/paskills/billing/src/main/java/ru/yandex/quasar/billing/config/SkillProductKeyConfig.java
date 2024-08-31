package ru.yandex.quasar.billing.config;

import lombok.Data;

@Data
public class SkillProductKeyConfig {
    private final String skillUuid;
    private final String productUuid;
}
