package ru.yandex.alice.kronstadt.core.tvm

/**
 * All implementations of the interface are found and used to configure TvmClient
 */
interface TvmDestinationRegistrar {
    fun register(aliases: MutableMap<String, Int>)
}
