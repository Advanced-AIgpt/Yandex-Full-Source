package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonValue;
import lombok.Data;

import ru.yandex.alice.kronstadt.core.utils.StringEnum;

@Data
public class ActivateSkillProduct {
    @JsonProperty("activation_type")
    private final ActivationType activationType;

    public enum ActivationType implements StringEnum {
        MUSIC("music");

        private final String value;

        ActivationType(String value) {
            this.value = value;
        }

        @JsonValue
        @Override
        public String value() {
            return value;
        }
    }
}
