package ru.yandex.alice.kronstadt.core.input

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame

sealed class Input {
    //SemanticFrames

    abstract val semanticFrames: List<SemanticFrame>

    open fun isCallback(clazz: Class<out CallbackDirective>): Boolean {
        return false
    }

    inline fun <reified T : CallbackDirective> isCallback(): Boolean = isCallback(T::class.java)

    open fun <T : CallbackDirective> getDirective(clazz: Class<T>): T {
        throw IllegalStateException("wrong input type")
    }

    data class Unknown(val name: String) : Input() {
        override val semanticFrames: List<SemanticFrame>
            get() = listOf()
    }

    data class Suggest(
        override val originalUtterance: String,
        override val normalizedUtterance: String,
        override val semanticFrames: List<SemanticFrame> = listOf(),
    ) : Input(), UtteranceInput

    data class Text(
        override val originalUtterance: String,
        override val normalizedUtterance: String,
        override val semanticFrames: List<SemanticFrame> = listOf(),
    ) : Input(), UtteranceInput

    data class Music(
        val musicResult: MusicResult,
        val musicData: MusicData,
        override val semanticFrames: List<SemanticFrame> = listOf(),
    ) : Input() {

        data class MusicData(
            val musicId: String,
            val url: String,

            ) {
            companion object {
                val EMPTY = MusicData("", "")
            }
        }

        enum class MusicResult(val code: String) {
            SUCCESS("success"), NOT_MUSIC("not-music");
        }
    }

    data class Callback(
        val directive: CallbackDirective,
        override val semanticFrames: List<SemanticFrame> = listOf()
    ) : Input() {
        override fun isCallback(clazz: Class<out CallbackDirective>): Boolean {
            return clazz.isAssignableFrom(directive.javaClass)
        }

        override fun <T : CallbackDirective> getDirective(clazz: Class<T>): T {
            return clazz.cast(directive)
        }
    }
}
