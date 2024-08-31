package ru.yandex.quasar.billing.filter;


import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import org.springframework.core.annotation.AliasFor;
import org.springframework.web.bind.annotation.Mapping;

import ru.yandex.quasar.billing.services.tvm.TvmClientName;

/**
 * Annotation to be used with {@link Mapping} methods.
 * The annotation enables check (see {@link TvmServiceAuthorizationInterceptor}) that TVM Service Ticket
 * is provided in headers of the request.
 * If {@link #allowed} list is not empty, then only specified TvmClients requests are permited,
 * all the others are forbidden.
 */
@Target({ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@Documented
public @interface TvmRequired {
    /**
     * List of allowed tvm clients.
     */
    @AliasFor("value")
    TvmClientName[] allowed() default {};

    @AliasFor("allowed")
    TvmClientName[] value() default {};
}
