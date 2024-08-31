package ru.yandex.alice.paskill.dialogovo.utils;

import javax.validation.ConstraintValidator;
import javax.validation.ConstraintValidatorContext;

import com.google.common.base.Charsets;

public class SizeInBytesValidator implements ConstraintValidator<SizeInBytes, String> {
    private int min;
    private int max;

    @Override
    public void initialize(SizeInBytes attr) {
        this.min = attr.min();
        this.max = attr.max();
    }

    @Override
    public boolean isValid(String value, ConstraintValidatorContext ctx) {
        if (value == null) {
            return true;
        }

        var length = value.getBytes(Charsets.UTF_8).length;
        return min <= length && length <= max;
    }
}
