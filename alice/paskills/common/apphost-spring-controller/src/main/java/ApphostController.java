package ru.yandex.alice.paskills.common.apphost.spring;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import org.springframework.core.annotation.AliasFor;
import org.springframework.stereotype.Component;

/**
 * Apphost controller may contain methods annotated with {@link ApphostHandler}
 */
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Component
public @interface ApphostController {
    @AliasFor("path")
    String value() default "";

    @AliasFor("value")
    String path() default "";
}
