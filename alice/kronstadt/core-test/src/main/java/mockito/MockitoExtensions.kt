package ru.yandex.alice.kronstadt.test.mockito

import org.mockito.ArgumentMatchers

/**
 * Until Mockito-kotlin is not added to contrib
 */

/** Object argument that is equal to the given value. */
fun <T> eq(value: T): T {
    return ArgumentMatchers.eq(value) ?: value
}

/**  Object argument that is the same as the given value. */
fun <T> same(value: T): T {
    return ArgumentMatchers.same(value) ?: value
}

/**  Any object */
fun <T> any(type: Class<T>): T =
    ArgumentMatchers.any(type)
