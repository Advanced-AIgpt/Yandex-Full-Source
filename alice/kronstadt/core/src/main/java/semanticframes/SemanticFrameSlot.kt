package ru.yandex.alice.kronstadt.core.semanticframes

import com.fasterxml.jackson.annotation.JsonIgnore
import java.util.Optional

data class SemanticFrameSlot @JvmOverloads constructor(
    val name: String,
    val type: String?,
    val value: String,
    val acceptedTypes: List<String> = listOf()
) {

    @JsonIgnore
    fun getTypeO() = Optional.ofNullable(type)

    companion object {
        @JvmStatic
        fun create(slotType: String, value: String): SemanticFrameSlot {
            return SemanticFrameSlot(slotType, null, value, emptyList())
        }

        @JvmStatic
        fun create(slotType: String, value: String, type: String): SemanticFrameSlot {
            return SemanticFrameSlot(slotType, type, value, emptyList())
        }
    }
}
