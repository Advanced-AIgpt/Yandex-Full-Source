package ru.yandex.alice.paskill.dialogovo.megamind

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.DefaultIrrelevantResponse
import ru.yandex.alice.kronstadt.core.PHRASES
import ru.yandex.alice.kronstadt.core.PREFIXES
import java.util.Random

internal class DefaultIrrelevantResponseTest {
    @Test
    fun testThousandPhrases() {
        val allowedPhrases: MutableSet<String> = HashSet()
        for (prefix in PREFIXES) {
            for (phrase in PHRASES) {
                allowedPhrases.add(prefix + phrase)
            }
        }
        val random = Random(42)
        for (i in 0..999) {
            val response = DefaultIrrelevantResponse.create<Any>("test_intent", random, true)
            val generatedPhrase = response.body.layout.outputSpeech
            Assertions.assertTrue(allowedPhrases.contains(generatedPhrase))
        }
    }
}
