package ru.yandex.alice.paskills.common.billing.model.api;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;

public class UserSkillProductActivationResult {
    @JsonProperty("activation_result")
    private final ActivationResult activationResult;

    @JsonProperty("user_skill_product")
    private final SkillProductItem skillProduct;

    public UserSkillProductActivationResult(
            @JsonProperty("activation_result") ActivationResult activationResult,
            @JsonProperty("user_skill_product") SkillProductItem skillProduct
    ) {
        this.activationResult = activationResult;
        this.skillProduct = skillProduct;
    }

    public ActivationResult getActivationResult() {
        return activationResult;
    }

    public SkillProductItem getSkillProduct() {
        return skillProduct;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        UserSkillProductActivationResult that = (UserSkillProductActivationResult) o;

        if (activationResult != that.activationResult) {
            return false;
        }
        return skillProduct.equals(that.skillProduct);
    }

    @Override
    public int hashCode() {
        int result = activationResult.hashCode();
        result = 31 * result + skillProduct.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "UserSkillProductActivationResult{" +
                "activationResult=" + activationResult +
                ", skillProduct=" + skillProduct +
                '}';
    }

    public enum ActivationResult {
        SUCCESS("success"),
        ALREADY_ACTIVATED("already_activated");

        private final String code;

        ActivationResult(String code) {
            this.code = code;
        }

        @JsonValue
        public String getCode() {
            return code;
        }
    }
}
