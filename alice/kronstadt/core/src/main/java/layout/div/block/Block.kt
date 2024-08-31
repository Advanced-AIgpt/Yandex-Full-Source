package ru.yandex.alice.kronstadt.core.layout.div.block

import com.fasterxml.jackson.annotation.JsonIgnore
import ru.yandex.alice.kronstadt.core.layout.div.DivAction

interface Block {
    val action: DivAction?
    val type: BlockType

    @get:JsonIgnore
    val subBlocks: Collection<Block>
        get() = listOf()
}
