package ru.yandex.alice.paskill.dialogovo.utils;

import java.util.regex.Pattern;

import javax.validation.ConstraintValidator;
import javax.validation.ConstraintValidatorContext;

public class MdsImageIdValidator implements ConstraintValidator<MdsImageId, String> {

    private static final Pattern PATTERN = Pattern.compile("^\\d+/\\S+$");

    @Override
    public boolean isValid(String value, ConstraintValidatorContext ctx) {
        if (value == null) {
            return true;
        }

        // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/external_skill/validator.cpp?rev=4590143#L153
        // 64 is magic! just to prevent sending too long imageId
        if (value.length() > 64) {
            return false;
        }

        return PATTERN.matcher(value).matches();
    }
}
