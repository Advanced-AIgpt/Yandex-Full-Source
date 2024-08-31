package ru.yandex.alice.kronstadt.core

data class ScenarioMeta
@JvmOverloads constructor(
    val name: String,
    val megamindName: String = name.split('_')
        .joinToString(separator = "") { it.replaceFirstChar { c -> c.titlecaseChar() } },
    val productScenarioName: String = name,

    // override scenario megamind path segment, name will be used by default
    val mmPath: String? = null,
    val alternativePaths: Set<String> = setOf(),
    // values is used by graph_generator
    val useDivRenderer: Boolean = false,
)
