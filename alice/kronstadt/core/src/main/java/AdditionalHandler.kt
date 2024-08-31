package ru.yandex.alice.kronstadt.core

import org.springframework.core.annotation.AliasFor

/**
 * Annotated method is treated as additional apphost-handler with signature similar to setup method.
 * Such nodes may be used for intermediate processing of request in grpc handler
 */
annotation class AdditionalHandler(
    @get:AliasFor("value") val path: String = "",
    @get:AliasFor("path") val value: String = ""
)
