package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import org.springframework.core.annotation.AliasFor;

/**
 * Annotation to define an apphost handler method
 */
@Target({ElementType.TYPE, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@Documented
public @interface ApphostHandler {

    @AliasFor("path")
    String value() default "";

    @AliasFor("value")
    String path() default "";
}
