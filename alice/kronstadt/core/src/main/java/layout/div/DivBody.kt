package ru.yandex.alice.kronstadt.core.layout.div;

import com.fasterxml.jackson.annotation.JsonInclude
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.layout.Card
import ru.yandex.alice.kronstadt.core.layout.div.block.Block
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto

@JsonInclude(JsonInclude.Include.NON_EMPTY)
data class DivBody(
    val background: List<DivBackground> = listOf(),
    val states: List<DivState> = listOf(),
): Card {

    override fun toProto(ctx: ToProtoContext, protoUtil: ProtoUtil): ResponseProto.TLayout.TCard {
        for (state in states) {
            rewriteState(state, ctx)
        }
        return ResponseProto.TLayout.TCard.newBuilder()
            .setDivCard(protoUtil.objectToStruct(this))
            .build()
    }

    private fun rewriteState(state: DivState, ctx: ToProtoContext) {
        state.action?.let { ctx.rewriteAction(it) }
        for (block in state.blocks) {
            rewriteBlock(block, ctx)
        }
    }

    private fun rewriteBlock(block: Block, ctx: ToProtoContext) {
        block.action?.let { ctx.rewriteAction(it) }
        block.subBlocks.forEach { subBlock: Block -> rewriteBlock(subBlock, ctx) }
    }

    class DivBodyBuilder(
        var background: MutableList<DivBackground> = mutableListOf(),
        var states: MutableList<DivState> = mutableListOf(),
    ): Card.Builder<DivBody> {

        override fun build(): DivBody {
            return DivBody(background, states)
        }

        fun background(backgrounds: MutableList<DivBackground>): DivBodyBuilder {
            this.background = backgrounds
            return this
        }

        fun states(states: MutableList<DivState>): DivBodyBuilder {
            this.states = states
            return this
        }

    }

    companion object {
        @JvmStatic
        fun builder(): DivBodyBuilder {
            return DivBodyBuilder()
        }
    }

}
