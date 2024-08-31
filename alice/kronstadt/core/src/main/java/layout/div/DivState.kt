package ru.yandex.alice.kronstadt.core.layout.div

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.annotation.JsonProperty
import ru.yandex.alice.kronstadt.core.layout.div.block.Block

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class DivState(
    @JsonProperty("state_id")
    val stateId: Int,
    val blocks: List<Block> = listOf(),
    val action: DivAction? = null,
)
