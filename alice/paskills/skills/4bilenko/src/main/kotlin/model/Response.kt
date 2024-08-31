package ru.yandex.alice.paskills.skills.bilenko.model

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.databind.node.ObjectNode

/**
 * Created by Pavel Kaplya on 21.03.2020.
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
data class WebhookResponse<S,A>(
    val response: ResponseBody,
    //val sessionState: Map<String, Any> = mapOf(),
    val sessionState: S? = null,
    val applicationState: A? = null,
    val userStateUpdate: Map<String, Any?> = mapOf(),
    val version: String = "1.0"
)

@JsonInclude(JsonInclude.Include.NON_NULL)
data class Button(
    val title: String,
    val payload: Any? = null,
    val url: String? = null,
    val hide: Boolean = false,
)

data class Card(
    val type: String,
    val imageId: String,
    val title: String? = null,
    val description: String? = null,
    val button: Button? = null,
)

@JsonInclude(JsonInclude.Include.NON_ABSENT)
data class ResponseBody(
    val text: String,
    val tts: String = text,
    val card: Card? = null,
    val shouldListen: Boolean = true,
    val endSession: Boolean = false,
    val buttons: List<Button> = emptyList(),
)

interface SessionState

object Empty

data class Nlg(
    val text: String,
    val tts: String = text
)

fun <S, A> withText(
    text: String,
    tts: String? = text,
    sessionState: S? = null,
    applicationState: A? = null,
    endSession: Boolean = false,
    suggests: List<String> = emptyList(),
) = WebhookResponse(
    response = ResponseBody(
        text = text,
        tts = tts ?: text,
        endSession = endSession,
        buttons = suggests.map { Button(it, hide = true) }
    ),
    sessionState = sessionState,
    applicationState = applicationState,
)

typealias ImageId = String

fun <S, A> withBigImage(
    text: String,
    imageId: ImageId,
    tts: String? = text,
    sessionState: S? = null,
    applicationState: A? = null,
    imageTitle: String? = null,
    imageDescription: String? = null,
    endSession: Boolean = false,
    suggests: List<String> = emptyList(),
) = WebhookResponse(
    response = ResponseBody(
        text = text,
        tts = tts ?: text,
        card = Card(type = "BigImage", imageId = imageId, title = imageTitle, description = imageDescription),
        endSession = endSession,
        buttons = suggests.map { Button(it, hide = true) }
    ),
    sessionState = sessionState,
    applicationState = applicationState,
)

fun <S, A>withText(
    nlg: Nlg,
    sessionState: S? = null,
    applicationState: A? = null,
    endSession: Boolean = false,
    suggests: List<String> = emptyList(),
) = withText(nlg.text, nlg.tts, sessionState, applicationState, endSession, suggests)

fun <S, A> WebhookResponse<S, A>.storeUserKey(key: String, payload: Any?): WebhookResponse<S, A>  {
    val newUserState = this.userStateUpdate.plus(key to payload).toMap()
    return WebhookResponse(
        response = response,
        sessionState = sessionState,
        userStateUpdate = newUserState,
        version = version
    )
}

fun <S, A> WebhookResponse<S, A>.storeUserKey(predicate: () -> Boolean, key: String, payload: Any?): WebhookResponse<S, A> {
    if (predicate.invoke()) {
        return storeUserKey(key, payload)
    }
    return this
}

fun <S, A>WebhookResponse<S, A>.storeUserKey(currentUserState: Map<String,Any?>, key: String, payload: Any?): WebhookResponse<S, A> {
    val currentValue = currentUserState[key]
    if (payload == null || currentValue != payload) {
        return storeUserKey(key, payload)
    }
    return this
}
