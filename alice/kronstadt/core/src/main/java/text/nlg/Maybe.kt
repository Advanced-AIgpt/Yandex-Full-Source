package ru.yandex.alice.kronstadt.core.text.nlg

import java.util.Random

class Maybe(
    private val enabled: Boolean,
    private val nlg: Nlg
) : Macros {

    constructor(random: Random, perc: Double, nlg: Nlg) : this(random.nextDouble() <= perc, nlg)

    fun or(): Maybe {
        return Maybe(!enabled, nlg)
    }

    fun tts(tts: String): Maybe {
        if (enabled) {
            nlg.tts(tts)
        }
        return this
    }

    override fun end(): Nlg {
        return nlg
    }

    fun ttsPause(ms: Int): Maybe {
        if (enabled) {
            nlg.ttsPause(ms)
        }
        return this
    }
}
