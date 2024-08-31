package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import com.fasterxml.jackson.databind.JsonNode;

/**
 * Stub class for new builtin entity types.
 * Should not be serialized to webhook request
 */
public class UnknownBuiltinEntity extends NluEntity {

    private final String value;

    public UnknownBuiltinEntity(int begin, int end, String type, JsonNode value) {
        this(begin, end, type, value.toString());
    }

    public UnknownBuiltinEntity(int begin, int end, String type, String value) {
        super(begin, end, type);
        this.value = value;
    }

    @Override
    public Object getValue() {
        return value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new UnknownBuiltinEntity(this.getTokens().getStart(), this.getTokens().getEnd(), this.getType(), value);
    }
}
