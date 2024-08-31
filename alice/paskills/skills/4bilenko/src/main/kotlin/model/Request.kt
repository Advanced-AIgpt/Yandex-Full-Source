package ru.yandex.alice.paskills.skills.bilenko.model

import com.fasterxml.jackson.databind.node.ObjectNode

/**
 * Created by Pavel Kaplya on 21.03.2020.
 */

data class WebhookRequest<SessionState, AppState>(
    val request: Request,
    val meta: Meta,
    val session: Session,
    val state: State<SessionState, AppState>?,
    val version: String
)

fun <S,A> WebhookRequest<S,A>.authorized() = session.user != null
fun <S,A> WebhookRequest<S,A>.intentSlot(intent: String, slotName: String): Slot? =
    request.nlu?.intents?.get(intent)?.slots?.get(slotName)

enum class RequestType { SimpleUtterance, ButtonPressed }

data class Request(
    val type: RequestType,
    val command: String,
    val originalUtterance: String,
    val payload: String? = null,
    val nlu: Nlu? = Nlu()
)

data class Nlu(val intents: Map<String, Intent> = mapOf())

data class Intent(val slots: Map<String, Slot> = mapOf())

data class Slot(val type: String, val value: String)

data class Meta(
    val locale: String,
    val timezone: String,
    val clientId: String,
    val interfaces: Map<String, Any>
)

data class Session(
    val new: Boolean,
    val messageId: Long,
    val sessionId: String,
    val application: Application,
    val skillId: String,
    val user: User? = null
)

data class Application(val applicationId: String)

data class User(val userId: String)

data class State<SessionState, AppState>(
    val session: SessionState? = null,
    val application: AppState? = null,
    val user: ObjectNode? = null
)
