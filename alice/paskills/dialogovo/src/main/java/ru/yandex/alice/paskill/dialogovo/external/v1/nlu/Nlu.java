package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import java.util.Collections;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.With;

import ru.yandex.alice.paskill.dialogovo.external.v1.nlu.json.IntentDeserializer;

@Data
@Builder
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_ABSENT)
public class Nlu {
    public static final Nlu EMPTY = new Nlu(Collections.emptyList(), Collections.emptyList(), Collections.emptyMap());

    private final List<String> tokens;
    private final List<NluEntity> entities;
    @With
    @JsonDeserialize(using = IntentDeserializer.class)
    @Nullable  // intents can be null if NLU object was created from NER service response
    private final Map<String, Intent> intents;

    public Map<String, Intent> getIntents() {
        return intents != null ? intents : Collections.emptyMap();
    }

}
