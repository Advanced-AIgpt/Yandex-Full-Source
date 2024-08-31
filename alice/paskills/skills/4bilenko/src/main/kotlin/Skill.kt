package ru.yandex.alice.paskills.skills.bilenko

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.annotation.JsonTypeInfo
import com.fasterxml.jackson.databind.node.ObjectNode
import ru.yandex.alice.paskills.skills.bilenko.model.WebhookResponse
import ru.yandex.alice.paskills.skills.bilenko.model.withText

/**
 * Created by Pavel Kaplya on 19.04.2020.
 */


@JsonTypeInfo(use = JsonTypeInfo.Id.NAME,
    include = JsonTypeInfo.As.PROPERTY,
    defaultImpl = SessionState.EmptyState::class,
    visible = true)
sealed class SessionState {
    @JsonProperty("@type")
    val type = this::class.simpleName

    object EmptyState : SessionState()

}

data class OfferMoreState(val shown: List<String>) : SessionState() {
    fun withShown(shownWish: String) = this.copy(shown = this.shown.plus(shownWish))
}

class AllShownState : SessionState()

typealias AudioId = String

const val skillId = "2ec0f6f8-52e2-4978-981b-c664a9e0842e";

data class Wish(
    val from: String,
    val fromTts: String = from,
    //val text: String,
    val audioId: AudioId,
) {
    fun opusTag() = "<speaker audio=\"dialogs-upload/$skillId/$audioId.opus\">"
}

val texts = listOf(
    Wish(from = "Андрея Холодного", audioId = "e9abca6a-3939-4403-ac96-9bac99b7b334"),
    Wish(from = "Полины Жиналиевой", fromTts = "Полины Жинал+иевой", audioId = "6a1e7d27-fef6-4e5b-a4d2-cf695872e356"),
    Wish(from = "Елены Бондарь", fromTts = "Елены Б+ондарь", audioId = "c8f2c21b-738a-4634-92d4-8cc526179959"),
    Wish(from = "Елены Чернышевой", fromTts = "Елены Чернышёвой", audioId = "b7bd9b7b-b83d-434b-af2c-1de912564520"),
    Wish(from = "Команды перевода",/* fromTts = "Елены Чернышёвой",*/ audioId = "a5431cf4-552c-4a78-8402-94f8d76ebc60"),
    Wish(from = "Дани Колесникова",/* fromTts = "Елены Чернышёвой",*/ audioId = "0efa7324-37bf-4120-839c-3ca075f8c1f0"),
    Wish(from = "Андрея Законова", fromTts = "Андрея Зак+онова", audioId = "85d20b5a-249e-422c-ba28-bcfaa4db2d34"),
    Wish(from = "Кости Лахмана", fromTts = "Кости Л+ахмана", audioId = "3a3a39aa-d9bb-4b7d-b2fa-33c675880b8f"),
    Wish(from = "Операторов терменвокса", fromTts = "операторов терменв+окса", audioId = "fa68e55a-e14b-4152-9210-275b7496c03e"),
    Wish(from = "дизайнеров Сергея и Насти", fromTts = "дизайнеров Сергея и Насти", audioId = "1f98c323-585c-4489-9a5c-af4626d81dea"),
    Wish(from = "Стаса Кириллова", audioId = "5c691ed1-151b-4c95-bad2-d09bc45a891f"),
    Wish(from = "Леши Гусакова", audioId = "8d670a86-20ac-4405-b6b8-2b878cee4dcf"),
    Wish(from = "Миши Ройзнера", fromTts = "Миши Р+ойзнэра", audioId = "73645e73-d0c3-40b3-a542-cf266a0169e1"),
    Wish(from = "Гали Тимошевской", fromTts = "Гали Тимош+евской", audioId = "d76b389c-5cc8-4fa9-b36b-dbe0e5d4471b"),
    Wish(from = "Сергея Овчаренко", audioId = "87a436ef-4b6c-4232-a823-83165320d7c7"),

    )

fun dispatcher(): Dispatcher<SessionState, ObjectNode> = configure {
    onNewSession {
        withText(
            text = listOf(
                "Привет Миша! Команда Яндекса подготовила для тебя несколько теплых слов. Хочешь послушать?",
                "Привет Миша! Этот навык - подарок тебе от команды Диалогов, в нем команда Яндекса оставила для тебя несколько теплых слов. Хочешь послушать?",
            ).random(),
            sessionState = OfferMoreState(listOf()),
            suggests = listOf("давай", "еще", "хватит"),
        )
    }

    onSessionStateAnd(OfferMoreState::class) {
        onIntents("tell_more", "listen") { tellMore((state!!.session as OfferMoreState).shown) }
        onAgree { tellMore((state!!.session as OfferMoreState).shown) }
        onRepeatAnd {
            onCondition({ (state!!.session as OfferMoreState).shown.isNotEmpty() }) {
                val shown = (state!!.session as OfferMoreState).shown
                renderWish(wish = texts.find { it.from == shown.last() }!!, shown = shown)
            }
        }
        onIntent("start_again") { tellMore(listOf()) }
        onReject {
            withText(text = "Ну ладно. Удачи, Миша, и спасибо за всё.", endSession = true)
        }
    }
    onSessionStateAnd(AllShownState::class) {
        onIntents("start_again", "listen") { tellMore(listOf()) }
        onAgree { tellMore(listOf()) }
        onReject {
            withText(text = "Ну ладно. Удачи, Миша, и до новых встреч.", endSession = true)
        }
    }

}

private fun tellMore(shown: List<String>): WebhookResponse<SessionState, ObjectNode> {
    //val offerMoreState =

    val wish = texts
        .filterNot { shown.contains(it.from) }
        .randomOrNull()
    return if (wish != null) {
        renderWish(wish, shown.plus(wish.from))
    } else {
        withText(text = "Кажется это все. Начнем с начала?", sessionState = AllShownState(),
            suggests = listOf("с начала"))
    }
}

private fun renderWish(wish: Wish, shown: List<String>): WebhookResponse<SessionState, ObjectNode> =
    withText(
        text = "Пожелание от ${wish.from}. Хочешь послушать еще?",
        tts = "пожелание от ${wish.fromTts}: ${wish.opusTag()}. Хочешь послушать еще?",
        sessionState = OfferMoreState(shown),
        suggests = listOf("еще", "хватит"),
    )
