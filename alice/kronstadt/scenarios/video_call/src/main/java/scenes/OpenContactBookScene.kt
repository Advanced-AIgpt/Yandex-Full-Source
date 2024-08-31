package ru.yandex.alice.kronstadt.scenarios.video_call.scenes

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.scenarios.video_call.OPEN_CONTACTS_BOOK_INTENT

@Component
object OpenContactBookScene : AbstractNoargScene<Any>("open_contact_book_scene") {

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        return RunOnlyResponse(
            layout = Layout.textWithOutputSpeech(
                shouldListen = false,
                textWithTts = TextWithTts("Тут будут контакты"),
                directives = listOf()
            ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = OPEN_CONTACTS_BOOK_INTENT),
            isExpectsRequest = false
        )
    }
}
