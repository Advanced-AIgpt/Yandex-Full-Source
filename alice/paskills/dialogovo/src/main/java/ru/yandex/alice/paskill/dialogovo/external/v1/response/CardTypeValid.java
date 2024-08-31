package ru.yandex.alice.paskill.dialogovo.external.v1.response;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import javax.validation.Constraint;
import javax.validation.ConstraintValidator;
import javax.validation.ConstraintValidatorContext;
import javax.validation.Payload;

import static java.lang.annotation.ElementType.FIELD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

@Target({FIELD})
@Retention(RUNTIME)
@Documented
@Constraint(validatedBy = CardTypeValid.CardTypeValidator.class)
public @interface CardTypeValid {

    String message() default "некорректный тип карточки";

    Class<?>[] groups() default {};

    Class<? extends Payload>[] payload() default {};

    class CardTypeValidator implements ConstraintValidator<CardTypeValid, CardType> {
        @Override
        public void initialize(CardTypeValid constraintAnnotation) {

        }

        @Override
        public boolean isValid(CardType value, ConstraintValidatorContext context) {
            return value != null && value != CardType.INVALID_CARD;
        }
    }
}
