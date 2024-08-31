package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.databind.node.ObjectNode
import kotlinx.coroutines.runBlocking
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.alice.paskills.skills.bilenko.model.*
import ru.yandex.alice.paskills.skills.bilenko.model.Application

class SkillTest {

    val dispatcher = dispatcher()

    @Test
    internal fun newSession() {
        val response = runBlocking {
            dispatcher.handle(newSessionEmptyRequest)
        }
        Assertions.assertTrue(response.sessionState is OfferMoreState)
        assertEquals(listOf<String>(), (response.sessionState as OfferMoreState).shown)
    }

    @Test
    internal fun `"давай" после приветствия`() {
        val response = runBlocking {
            val request = Request(
                type = RequestType.SimpleUtterance,
                command = "давай",
                originalUtterance = "давай",
                nlu = Nlu(intents = mapOf("YANDEX.CONFIRM" to Intent()))
            )
            val webhookRequest = inSession(request)
                .withSessionState(OfferMoreState(listOf()))
            return@runBlocking dispatcher.handle(webhookRequest)
        }
        Assertions.assertTrue(response.sessionState is OfferMoreState)
        assertEquals(1, (response.sessionState as OfferMoreState).shown.size)
    }

    @Test
    internal fun `"еще" после приветствия`() {
        val response = runBlocking {
            val request = Request(
                type = RequestType.SimpleUtterance,
                command = "еще",
                originalUtterance = "еще",
                nlu = Nlu(intents = mapOf("tell_more" to Intent()))
            )
            val webhookRequest = inSession(request)
                .withSessionState(OfferMoreState(listOf("Андрей Холодный")))
            return@runBlocking dispatcher.handle(webhookRequest)
        }
        Assertions.assertTrue(response.sessionState is OfferMoreState)
        assertEquals(2, (response.sessionState as OfferMoreState).shown.size)
    }

}

fun <S, A> WebhookRequest<S, A>.withSessionState(newState: S) =
    this.copy(state = this.state?.copy(session = newState) ?: State(session = newState))

fun newSession(request: Request) = request(request, true)
fun inSession(request: Request) = request(request, false)

fun request(request: Request, newSession: Boolean): WebhookRequest<SessionState, ObjectNode> = WebhookRequest(
    request = request,
    meta = Meta(
        locale = "ru-RU",
        timezone = "UTC",
        clientId = "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
        interfaces = mapOf("screen" to Empty, "payments" to Empty, "account_linking" to Empty)
    ),
    session = Session(
        messageId = if (newSession) 0 else 1,
        sessionId = "09f5f3ca-0e0e-4e8b-8245-0853757e6def",
        skillId = "2ec0f6f8-52e2-4978-981b-c664a9e0842e",
        application = Application("E5914E2407F03AEBF2B0F856A7D1B1F83D5D44CCE543E22CB61667498B1DD28F"),
        new = newSession
    ),
    state = null,
    version = "1.0"
)

val emptyRequest = Request(
    command = "",
    originalUtterance = "",
    type = RequestType.SimpleUtterance,
)

val newSessionEmptyRequest = request(emptyRequest, true)


