package ru.yandex.alice.kronstadt.core.scenario

import java.util.concurrent.ConcurrentHashMap
import java.util.function.Function

internal class SimpleCache<K, V>(
    private val map: ConcurrentHashMap<K, V> = ConcurrentHashMap<K, V>(),
    private val factory: Function<K, V>
) {
    operator fun get(key: K): V = map.computeIfAbsent(key, factory)
}
