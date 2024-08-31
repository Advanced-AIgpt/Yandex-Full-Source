package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.util.HashMap;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonAnyGetter;
import com.fasterxml.jackson.annotation.JsonAnySetter;

import static ru.yandex.alice.paskill.dialogovo.external.v1.response.CardType.INVALID_CARD;

public class InvalidCard extends Card {

    private final Map<String, Object> fields = new HashMap<>();

    InvalidCard() {
        super(INVALID_CARD);
    }

    @JsonAnyGetter
    public Map<String, Object> getFields() {
        return fields;
    }

    @JsonAnySetter
    public void setFields(Map<String, Object> fields) {
        this.fields.putAll(fields);
    }
}
