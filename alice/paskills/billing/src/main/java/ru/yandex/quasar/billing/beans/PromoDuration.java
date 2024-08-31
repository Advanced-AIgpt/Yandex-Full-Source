package ru.yandex.quasar.billing.beans;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;

public enum PromoDuration {
    P1M("1 месяц", new PromoDurationValue(1, Unit.M)),
    P3M("3 месяца", new PromoDurationValue(3, Unit.M)),
    P4M("4 месяца", new PromoDurationValue(4, Unit.M)),
    P6M("6 месяцев", new PromoDurationValue(6, Unit.M)),
    P1Y("1 год", new PromoDurationValue(1, Unit.Y));

    private final String text;
    private final PromoDurationValue value;

    PromoDuration(String text, PromoDurationValue value) {
        this.text = text;
        this.value = value;
    }

    public String getText() {
        return text;
    }

    public PromoDurationValue getValue() {
        return value;
    }

    public enum Unit {
        M, Y
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class PromoDurationValue {
        private final int value;
        private final Unit unit;
    }


}
