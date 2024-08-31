package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

@EqualsAndHashCode(callSuper = true)
public class GeoNluEntity extends NluEntity {
    @Getter
    private final Value value;

    public GeoNluEntity(int begin, int end, Value value) {
        super(begin, end, BuiltinNluEntityType.GEO);
        this.value = value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new GeoNluEntity(getTokens().getStart(), getTokens().getEnd(), value);
    }

    @Data
    @Builder
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class Value {
        @Nullable
        private final String country;
        @Nullable
        private final String city;
        @Nullable
        private final String street;
        @JsonProperty("house_number")
        @Nullable
        private final String houseNumber;
        @Nullable
        private final String airport;
    }
}
