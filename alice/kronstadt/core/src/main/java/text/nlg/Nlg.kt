package ru.yandex.alice.kronstadt.core.text.nlg

import ru.yandex.alice.kronstadt.core.ActionRef
import ru.yandex.alice.kronstadt.core.layout.Button
import ru.yandex.alice.kronstadt.core.utils.SoundUtils.opusTts
import java.util.Optional
import java.util.Random

class Nlg(private val random: Random) {
    val text = StringBuilder()
    val tts = StringBuilder()
    val suggests: MutableList<Button> = mutableListOf()
    val actions: MutableMap<String, ActionRef> = hashMapOf()
    fun tts(tts: String): Nlg {
        this.tts.append(tts)
        return this
    }

    fun tts(ttsO: Optional<String>): Nlg {
        ttsO.ifPresent { str -> tts.append(str) }
        return this
    }

    fun opus(skillId: String, soundId: String): Nlg {
        tts.append(opusTts(skillId, soundId))
        return this
    }

    fun ttsPause(ms: Int): Nlg {
        tts.append("sil<[").append(ms).append("]>")
        return this
    }

    fun text(text: String): Nlg {
        this.text.append(text)
        return this
    }

    fun text(textO: Optional<String>): Nlg {
        textO.ifPresent { str -> text.append(str) }
        return this
    }

    fun ttsWithText(text: String): Nlg {
        this.text.append(text)
        tts.append(text)
        return this
    }

    fun suggest(suggest: Button): Nlg {
        suggests.add(suggest)
        return this
    }

    fun action(actionName: String, actionRef: ActionRef): Nlg {
        actions[actionName] = actionRef
        return this
    }

    fun actions(actions: Map<String, ActionRef>): Nlg {
        this.actions.putAll(actions)
        return this
    }

    fun maybe(prob: Double): Maybe {
        return Maybe(random, prob, this)
    }
}
