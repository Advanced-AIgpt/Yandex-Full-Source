package ru.yandex.alice.paskills.common.billing.model.api;

import javax.annotation.Nullable;
import javax.validation.constraints.Size;

import com.fasterxml.jackson.annotation.JsonProperty;

public class UserSkillProductActivationRequest {

    @Nullable
    @JsonProperty("skill_name")
    @Size(max = 2048)
    private final String skillName;

    @Nullable
    @JsonProperty("skill_image_url")
    @Size(max = 4096)
    private final String skillImageUrl;

    public UserSkillProductActivationRequest(
            @JsonProperty("skill_name") @Nullable String skillName,
            @JsonProperty("skill_image_url") @Nullable String skillImageUrl
    ) {
        this.skillName = skillName;
        this.skillImageUrl = skillImageUrl;
    }

    @Nullable
    public String getSkillName() {
        return skillName;
    }

    @Nullable
    public String getSkillImageUrl() {
        return skillImageUrl;
    }

    @Override
    public String toString() {
        return "UserSkillProductActivationRequest{" +
                "skillName='" + skillName + '\'' +
                ", skillImageUrl='" + skillImageUrl + '\'' +
                '}';
    }
}
