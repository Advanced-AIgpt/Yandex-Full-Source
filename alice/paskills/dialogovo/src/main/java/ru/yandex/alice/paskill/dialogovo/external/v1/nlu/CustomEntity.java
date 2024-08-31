package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

public class CustomEntity extends NluEntity {
    private final String value;

    public CustomEntity(int begin, int end, String type, String value) {
        super(begin, end, type);
        this.value = value;
    }

    public String getValue() {
        return value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new CustomEntity(getTokens().getStart(), getTokens().getEnd(), getType(), value);
    }
}
