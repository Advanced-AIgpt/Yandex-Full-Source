package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

@EqualsAndHashCode(callSuper = true)
public class FioNluEntity extends NluEntity {
    @Getter
    private final Value value;

    public FioNluEntity(int begin, int end, Value value) {
        super(begin, end, BuiltinNluEntityType.FIO);
        this.value = value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new FioNluEntity(getTokens().getStart(), getTokens().getEnd(), value);
    }

    @Data
    @Builder
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class Value {
        @Nullable
        @JsonProperty("first_name")
        private final String firstName;
        @Nullable
        @JsonProperty("patronymic_name")
        private final String patronymicName;
        @Nullable
        @JsonProperty("last_name")
        private final String lastName;
    }
}
