package ru.yandex.alice.paskills.common.billing.model.api;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

public class UserSkillProductsResult {

    @JsonProperty("user_skill_products")
    private final List<SkillProductItem> skillProducts;

    @JsonCreator(mode = JsonCreator.Mode.PROPERTIES)
    public UserSkillProductsResult(
            @JsonProperty("user_skill_products") List<SkillProductItem> skillProducts
    ) {
        this.skillProducts = skillProducts;
    }

    public List<SkillProductItem> getSkillProducts() {
        return skillProducts;
    }

    @Override
    public String toString() {
        return "UserSkillProductsResult{" +
                "skillProducts=" + skillProducts +
                '}';
    }
}
