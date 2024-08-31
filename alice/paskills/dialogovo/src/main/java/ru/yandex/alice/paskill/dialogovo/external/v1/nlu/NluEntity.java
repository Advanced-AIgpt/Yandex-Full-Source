package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json.NluEntityDeserializer;
import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json.NluEntitySerializer;

@Getter
@EqualsAndHashCode
@JsonSerialize(using = NluEntitySerializer.class)
@JsonDeserialize(using = NluEntityDeserializer.class)
public abstract class NluEntity {

    private final String type;
    private final PositionTokens tokens;

    @Nullable
    @JsonInclude(JsonInclude.Include.NON_EMPTY)
    @JsonProperty(value = "additional_values")
    private final List<NluEntity> additionalValues = new ArrayList<>();

    public NluEntity(int begin, int end, String type) {
        tokens = new PositionTokens(begin, end);
        this.type = type;
    }

    public void merge(NluEntity additionalValue) {
        additionalValues.add(additionalValue);
    }

    public String getType() {
        return this.type;
    }

    public abstract Object getValue();

    @Nullable
    public List<NluEntity> getAdditionalValues() {
        return additionalValues;
    }

    public abstract NluEntity withoutAdditionalValues();

    @Data
    public static class PositionTokens {
        private final int start;
        private final int end;
    }
}
