package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.FIELD, ElementType.TYPE_USE, ElementType.METHOD})
public @interface MaskSensitiveData {
}
