package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import java.math.BigDecimal;

import lombok.EqualsAndHashCode;
import lombok.Getter;

@EqualsAndHashCode(callSuper = true)
public class NumberNluEntity extends NluEntity {
    @Getter
    private final BigDecimal value;

    public NumberNluEntity(int begin, int end, BigDecimal value) {
        super(begin, end, BuiltinNluEntityType.NUMBER);
        this.value = value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new NumberNluEntity(this.getTokens().getStart(), this.getTokens().getEnd(), value);
    }
}
