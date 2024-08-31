package ru.yandex.alice.paskills.common.tvm.spring.handler;


import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import org.springframework.core.annotation.AliasFor;
import org.springframework.web.bind.annotation.Mapping;

/**
 * Annotation to be used with {@link Mapping} methods.
 * The annotation enables check (see {@link TvmAuthorizationInterceptor}) that TVM Service Ticket
 * is provided in headers of the request.
 * If {@link #allowed} list is not empty, then only specified TvmClients requests are permited,
 * all the others are forbidden.
 */
@Target({ElementType.METHOD, ElementType.TYPE})
@Retention(RetentionPolicy.RUNTIME)
@Documented
public @interface TvmRequired {
    /**
     * List of allowed tvm clients.
     */
    @AliasFor("value")
    String[] allowed() default {};

    @AliasFor("allowed")
    String[] value() default {};

    /**
     * Fall with 403 if user not authorized
     */
    boolean userRequired() default false;
}
