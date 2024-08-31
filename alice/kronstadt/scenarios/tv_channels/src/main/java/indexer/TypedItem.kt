package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty

data class TypedItem(
    val value: Any,
    @JsonIgnore private val types: Set<TypedItemType>,
) {
    constructor(value: Any, vararg type: TypedItemType) : this(value, setOf(*type))

    init {
        assert(types.isNotEmpty()) { "Types must not be empty" }
    }

    @JsonProperty("type")
    fun type(): String = "#" + types.joinToString(separator = "") { it.code }
}
