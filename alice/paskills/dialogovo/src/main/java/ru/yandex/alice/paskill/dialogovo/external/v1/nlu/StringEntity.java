package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import lombok.EqualsAndHashCode;
import lombok.Getter;

@EqualsAndHashCode(callSuper = true)
public class StringEntity extends NluEntity {

    @Getter
    private final String value;

    public StringEntity(int begin, int end, String value) {
        super(begin, end, BuiltinNluEntityType.STRING);
        this.value = value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new StringEntity(getTokens().getStart(), getTokens().getEnd(), value);
    }

}
