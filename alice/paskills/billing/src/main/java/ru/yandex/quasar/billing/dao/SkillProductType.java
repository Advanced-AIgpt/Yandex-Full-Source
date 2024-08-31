package ru.yandex.quasar.billing.dao;

import javax.annotation.Nonnull;

import ru.yandex.quasar.billing.util.HasCode;

public enum SkillProductType implements HasCode<String> {
    NON_CONSUMABLE("NON_CONSUMABLE");

    private final String code;
    SkillProductType(String code) {
        this.code = code;
    }

    @Nonnull
    @Override
    public String getCode() {
        return code;
    }
}
