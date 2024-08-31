package ru.yandex.alice.kronstadt.scenarios.video_call.utils

import org.apache.logging.log4j.util.Strings
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.text.OrdinalRenderer.renderOrdinal
import ru.yandex.alice.kronstadt.core.text.Phrases

private const val CALLING_HINT_PHRASE = "video_call.choose_contact_hint"
private const val TOKENS_COMBINATIONS_LIMIT = 7 // кол-во сочетаний из 7 по 2 = 21

@Component
class NluHints (
    @param:Qualifier("videoCallPhrases") private val phrases: Phrases,
) {

    fun makeChooseContactNluHint(
        frameName: String,
        contactName: String,
        displayPosition: Int): ActionRef.NluHint {

        val nluHintPhrases = with(contactName.split(' ')) {
            combineTokens(this).toMutableList().also {
                it.add((Strings.join(this, ' '))) // add full tokenized name
            }
        }
        nluHintPhrases.add((displayPosition + 1).toString())
        renderOrdinal(displayPosition + 1)?.apply { nluHintPhrases.add(this) }
        nluHintPhrases.addAll(nluHintPhrases.map { phrases.get(key = CALLING_HINT_PHRASE, variables = listOf(it))})

        return ActionRef.NluHint(
            frameName = frameName,
            phrases = nluHintPhrases
        )
    }


    private fun combineTokens(tokens: List<String>): List<String> {
        if (tokens.size <= 2) {
            return tokens
        }
        val tokenCombinations = tokens.toMutableList()
        tokenCombinations.add(Strings.join(tokens, ' '))

        val maxTokenNumber = minOf(tokens.size, TOKENS_COMBINATIONS_LIMIT)
        for (i in 0..maxTokenNumber) {
            for (j in i + 1 until maxTokenNumber)
                tokenCombinations.add("${tokens[i]} ${tokens[j]}")
        }
        return tokenCombinations
    }
}
