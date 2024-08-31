package ru.yandex.alice.paskills.common.billing.model.api;

import com.fasterxml.jackson.annotation.JsonProperty;

public class SkillProductItem {
    private final String uuid;
    private final String name;

    public SkillProductItem(
            @JsonProperty("uuid") String uuid,
            @JsonProperty("name") String name
    ) {
        this.uuid = uuid;
        this.name = name;
    }

    public String getUuid() {
        return uuid;
    }

    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return "SkillProductItem{" +
                "uuid='" + uuid + '\'' +
                ", name='" + name + '\'' +
                '}';
    }
}
