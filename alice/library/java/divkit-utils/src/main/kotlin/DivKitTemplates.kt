package ru.yandex.alice.divkit

import com.google.protobuf.Message
import com.yandex.div.dsl.context.CardWithTemplates
import ru.yandex.alice.library.client.protos.ClientInfoProto
import ru.yandex.alice.protos.data.language.Language
import java.time.Instant
import kotlin.reflect.KClass

interface BaseContext {
    val clientInfo: ClientInfoProto.TClientInfoProto
    val lang: Language.ELang
    val experiments: Set<String>
}

interface StaticContext : BaseContext
interface DynamicContext : BaseContext {
    val randomSeed: Long
    val serverTime: Instant
}

data class StaticContextImpl(
    override val clientInfo: ClientInfoProto.TClientInfoProto,
    override val lang: Language.ELang,
    override val experiments: Set<String>,
) : StaticContext

data class DynamicContextImpl(
    override val clientInfo: ClientInfoProto.TClientInfoProto,
    override val lang: Language.ELang,
    override val experiments: Set<String>,
    override val randomSeed: Long,
    override val serverTime: Instant,
) : DynamicContext, StaticContext

sealed interface DivKitTemplate<T : Message> {
    val templateDataClass: KClass<T>
}

/**
 * Idempotent templates allow caching but provide only
 */
interface IdempotentDivKitTemplate<T : Message> : DivKitTemplate<T> {
    fun render(ctx: StaticContext, data: T): CardWithTemplates
}

interface NonIdempotentDivKitTemplate<T : Message> : DivKitTemplate<T> {
    fun render(ctx: DynamicContext, data: T): CardWithTemplates
}



