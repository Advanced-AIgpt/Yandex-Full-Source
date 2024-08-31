package ru.yandex.alice.kronstadt.core

import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.Logger
import java.util.Optional
import java.util.function.Function

interface WithExperiments {
    val experiments: Set<String>

    fun hasExperiment(experiment: String): Boolean {
        return experiments.contains(experiment)
    }

    fun getExperimentStartWith(experiment: String): String? {
        return experiments.firstOrNull { exp -> exp.startsWith(experiment) }
    }

    fun getExperimentStartWithO(experiment: String): Optional<String> {
        return Optional.ofNullable(getExperimentStartWith(experiment))
    }

    fun <T : Any> getExperimentWithValueO(prefix: String, converter: Function<String, T>): Optional<T> {
        val experimentString = getExperimentStartWithO(prefix)
        if (experimentString.isPresent) {
            val stringValue = experimentString.get().replace(prefix, "")
            try {
                return Optional.of(converter.apply(stringValue))
            } catch (e: Exception) {
                logger.warn("Failed to parse experiment value from {}", stringValue)
            }
        }
        return Optional.empty()
    }

    fun <T : Any> getExperimentWithValue(prefix: String, defaultValue: T, converter: Function<String, T>): T {
        return getExperimentWithValueO(prefix, converter).orElse(defaultValue)
    }

    fun getExperimentWithValue(prefix: String, defaultValue: Double): Double {
        return getExperimentWithValue(prefix, defaultValue, { s: String -> s.toDouble() })
    }

    fun getExperimentWithValue(prefix: String, defaultValue: String): String {
        return getExperimentWithValue(prefix, defaultValue, { s: String -> s })
    }

    fun getExperimentWithValue(prefix: String, defaultValue: Int): Int {
        return getExperimentWithValue(prefix, defaultValue.toDouble()).toInt()
    }

    fun getExperimentsWithCommaSeparatedListValues(prefix: String, defaultValue: List<String>): List<String> {
        return experiments
            .filter { exp -> exp.startsWith(prefix) }
            .map { exp -> exp.replace(prefix, "") }
            .flatMap { values -> values.split(",") }
    }

    companion object {
        val logger: Logger = LogManager.getLogger()
    }
}
