package ru.yandex.alice.paskill.dialogovo.utils.validator;

import java.util.regex.Pattern;

import javax.validation.ConstraintValidator;
import javax.validation.ConstraintValidatorContext;

public class SizeWithoutTagsValidatorForCharSequence implements ConstraintValidator<SizeWithoutTags, CharSequence> {

    private static final Pattern PATTERN = Pattern.compile("<speaker.*?>", Pattern.CASE_INSENSITIVE);
    private int min;
    private int max;

    @Override
    public void initialize(SizeWithoutTags parameters) {
        min = parameters.min();
        max = parameters.max();
        validateParameters();
    }

    /**
     * Checks the length of the specified character sequence (e.g. string).
     *
     * @param charSequence               The character sequence to validate.
     * @param constraintValidatorContext context in which the constraint is evaluated.
     * @return Returns {@code true} if the string is {@code null} or the length of {@code charSequence} between the
     * specified
     * {@code min} and {@code max} values (inclusive), {@code false} otherwise.
     */
    @Override
    public boolean isValid(CharSequence charSequence, ConstraintValidatorContext constraintValidatorContext) {
        if (charSequence == null) {
            return true;
        }

        int length = PATTERN.matcher(charSequence).replaceAll("").trim().length();
        return length >= min && length <= max;
    }

    private void validateParameters() {
        if (min < 0) {
            throw new IllegalArgumentException("The min parameter cannot be negative.");
        }
        if (max < 0) {
            throw new IllegalArgumentException("The max parameter cannot be negative.");
        }
        if (max < min) {
            throw new IllegalArgumentException("The length cannot be negative.");
        }
    }
}
