package ru.yandex.alice.paskills.skills.bilenko

import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookRequest
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookResponse
import ru.yandex.alice.paskills.skills.bilenko.model.intentSlot
import ru.yandex.alice.paskills.skills.bilenko.model.withText
import kotlin.reflect.KClass

@Component
class Dispatcher<S, A>(private val handlers: List<Handle<S, A>> = listOf()) {

    suspend fun handle(request: WebhookRequest<S, A>): WebhookResponse<S, A> {
        val response = handlers
            .firstOrNull { handler -> handler.check.invoke(request) }?.process?.invoke(request)
        if (response == null) {
            println("fallback! state: [${request.state}], request: [${request.request.command}], intents: [${request.request.nlu?.intents}]")
            return withText("Я вас не поняла", sessionState = request.state?.session, applicationState = null)
        }
        return response
    }
}

typealias Checker<S, A> = suspend WebhookRequest<S, A>.() -> Boolean
typealias Processor<S, A> = suspend WebhookRequest<S, A>.() -> WebhookResponse<S, A>

val TRUE_CONDITION: Checker<*, *> = { true }

fun <S, A> Checker<S, A>.and(checker: Checker<S, A>): Checker<S, A> = {
    this@and.invoke(this) && checker.invoke(this)
}

interface Handle<S, A> {
    val check: Checker<S, A>
    val process: Processor<S, A>
}

private class HandleImpl<S, A>(
    override val check: Checker<S, A>, override val process: Processor<S, A>
) : Handle<S, A>

class Builder<S, A> {
    val handlers = mutableListOf<Handle<S, A>>()

    fun add(check: Checker<S, A>, action: Processor<S, A>) {
        handlers.add(HandleImpl(check, action))
    }

    fun onCondition(check: Checker<S, A>, action: Processor<S, A>): Builder<S, A> {
        add(check, action)
        return this
    }

    fun onFirstTimeWelcome(action: Processor<S, A>) = onCondition({ session.new && state?.user == null }, action)

    fun onReturnedWelcome(action: Processor<S, A>) = onCondition({ session.new && state?.user != null }, action)

    fun onNewSession(action: Processor<S, A>) = onCondition({ session.new }, action)

    fun onHelp(action: Processor<S, A>): Builder<S, A> = onIntent("YANDEX.HELP", action)
    fun onAgree(action: Processor<S, A>): Builder<S, A> = onIntent("YANDEX.CONFIRM", action)
    fun onReject(action: Processor<S, A>): Builder<S, A> = onIntent("YANDEX.REJECT", action)
    fun onRepeat(action: Processor<S, A>): Builder<S, A> = onIntent("YANDEX.REPEAT", action)
    fun onRepeatAnd(body: Builder<S, A>.() -> Builder<S, A>): Builder<S, A> = onIntentAnd("YANDEX.REPEAT", body)

    fun onFallback(action: Processor<S, A>) = onCondition(TRUE_CONDITION) {
        println("fallback! state: [${state}], request: [${request.command}], intents: [${request.nlu?.intents}]")
        action.invoke(this)
    }

    fun onNewSessionAnd(body: Builder<S, A>.() -> Builder<S, A>): Builder<S, A> {
        val check: Checker<S, A> = { session.new }
        val builder = Builder<S, A>()
        builder.body()
        builder.handlers.forEach { handler ->
            add(check.and(handler.check), handler.process)
        }
        return this
    }

    fun onSessionState(stateClazz: KClass<*>, action: Processor<S, A>) =
        this.onCondition({ stateClazz.isInstance(state?.session ?: false) }, action)

    fun onSessionStateAnd(stateClazz: KClass<*>, body: Builder<S, A>.() -> Builder<S, A>): Builder<S, A> {
        val builder = Builder<S, A>()
        builder.body()
        val check: Checker<S, A> = { stateClazz.isInstance(state?.session ?: false) }
        builder.handlers.forEach { handler ->
            this.add(check.and(handler.check), handler.process)
        }
        return this
    }

    fun onIntent(intent: String, action: Processor<S, A>): Builder<S, A> {
        val check: Checker<S, A> = { request.nlu?.intents?.containsKey(intent) == true }
        add(check, action)
        return this
    }
    fun onIntentAnd(intent: String, body: Builder<S, A>.() -> Builder<S, A>): Builder<S, A> {
        val builder = Builder<S, A>()
        builder.body()
        val check: Checker<S, A> = { request.nlu?.intents?.containsKey(intent) == true }
        builder.handlers.forEach { handler ->
            this.add(check.and(handler.check), handler.process)
        }
        return this
    }

    fun onIntents(vararg intents: String, action: Processor<S, A>): Builder<S, A> {
        val check: Checker<S, A> = {
            request.nlu?.intents?.isNotEmpty() ?: false &&
                setOf(*intents).any { request.nlu?.intents?.containsKey(it) == true }
        }
        add(check, action)
        return this
    }

    fun onIntentWithSlot(intent: String, slot: String, action: Processor<S, A>): Builder<S, A> {
        val check: Checker<S, A> = { intentSlot(intent, slot) != null }
        add(check, action)
        return this
    }

    fun build() = Dispatcher<S, A>(handlers.toList())
}

fun <S, A> configure(body: Builder<S, A>.() -> Builder<S, A>): Dispatcher<S, A> = Builder<S, A>().body().build()
