package ru.yandex.alice.paskill.dialogovo.scenarios.recipes.megamind.processors

import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.IrrelevantResponse
import ru.yandex.alice.kronstadt.core.ScenarioResponseBody
import ru.yandex.alice.kronstadt.core.layout.TextWithTts
import ru.yandex.alice.kronstadt.core.text.Phrases
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState
import java.util.Random

class WeAreNotCookingResponse(responseBody: ScenarioResponseBody<DialogovoState>) :
    IrrelevantResponse<DialogovoState>(responseBody) {

    @Component("weAreNotCookingResponseFactory")
    class Factory(@param:Qualifier("recipePhrases") private val phrases: Phrases) : PhrasesFactory<DialogovoState>() {

        override fun getPhrase(random: Random): TextWithTts {
            return phrases.getRandomTextWithTts("we_are_not_cooking", random)
        }
    }
}
