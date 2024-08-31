package ru.yandex.alice.kronstadt.core.convert.request

import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.context.annotation.ClassPathScanningCandidateComponentProvider
import org.springframework.core.type.filter.AssignableTypeFilter
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.AliceHandledException
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.kronstadt.core.convert.ProtoUtil
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective
import ru.yandex.alice.kronstadt.core.directive.Directive
import ru.yandex.alice.kronstadt.core.input.Input
import ru.yandex.alice.kronstadt.core.input.Input.Music.MusicData
import ru.yandex.alice.kronstadt.core.input.Input.Music.MusicResult
import ru.yandex.alice.kronstadt.core.semanticframes.SemanticFrame
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import ru.yandex.alice.megamind.protos.scenarios.RequestProto.TInput.TVoice
import ru.yandex.alice.megamind.protos.scenarios.directive.TCallbackDirective
import ru.yandex.alice.paskills.common.proto.utils.ProtoWarmupper

@Component
open class InputConverter(
    private val semanticFrameConverter: SemanticFrameConverter,
    private val protoUtil: ProtoUtil,
    objectMapper: ObjectMapper,
) : FromProtoConverter<RequestProto.TInput, Input> {
    private val name2DirectiveClass: Map<String, Class<CallbackDirective>>

    private val unknownCallbackAction =
        AnalyticsInfoAction("unknown_callback", "unknown_callback", "Неизвестный коллбек")

    init {
        val scanner = ClassPathScanningCandidateComponentProvider(false)
        scanner.addIncludeFilter(AssignableTypeFilter(CallbackDirective::class.java))
        val beans = scanner.findCandidateComponents("ru.yandex")
        this.name2DirectiveClass = beans
            .mapNotNull { bean -> bean.beanClassName }
            .map { className: String -> getClass(className) }
            .filter { c -> c.getAnnotation(Directive::class.java)?.value != null }
            .associateBy { c -> c.getAnnotation(Directive::class.java).value }
        ProtoWarmupper.warmupJackson(objectMapper, this.name2DirectiveClass.values.toSet())
    }

    override fun convert(src: RequestProto.TInput): Input {
        val semanticFrames: List<SemanticFrame> = src.semanticFramesList
            .map { semanticFrameConverter.convert(it) }

        return when {
            src.hasText() -> {
                val text = src.text
                return if (text.fromSuggest) {
                    Input.Suggest(
                        normalizedUtterance = text.utterance,
                        originalUtterance = text.rawUtterance,
                        semanticFrames = semanticFrames,
                    )
                } else Input.Text(
                    normalizedUtterance = text.utterance,
                    originalUtterance = text.rawUtterance,
                    semanticFrames = semanticFrames,
                )
            }
            src.hasVoice() -> {
                val voice = src.voice
                return Input.Text(
                    normalizedUtterance = voice.utterance,
                    originalUtterance = getOriginalUtteranceFromVoice(voice),
                    semanticFrames = semanticFrames,
                )
            }
            src.hasCallback() -> {
                return getCallbackInput(src.callback, semanticFrames)
            }
            src.hasMusic() -> {
                return convertMusicInput(src.music, semanticFrames)
            }
            else -> {
                logger.warn("unsupported input type {}", src.eventCase.name)
                Input.Unknown(src.eventCase.name)
            }
        }
    }

    private fun convertMusicInput(music: RequestProto.TInput.TMusic, semanticFrames: List<SemanticFrame>): Input {
        val result = music.musicResult.result
        return if (MusicResult.SUCCESS.code == result) {
            val musicData = music.musicResult.data
            val id = musicData.match.getFieldsOrThrow("id").stringValue
            val url = musicData.url
            Input.Music(MusicResult.SUCCESS, MusicData(id, url), semanticFrames)
        } else {
            Input.Music(MusicResult.NOT_MUSIC, MusicData.EMPTY, semanticFrames)
        }
    }

    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/library/request/event/voice_input_event.h?rev=5511511
    private fun getOriginalUtteranceFromVoice(voice: TVoice): String {
        if (voice.asrDataList.isEmpty()) {
            return voice.utterance
        }
        val firstAsrData = voice.asrDataList[0]
        if (firstAsrData.wordsList.isEmpty()) {
            return voice.utterance
        }

        return firstAsrData.wordsList
            .joinToString(" ") { it.value }
            .ifEmpty { voice.utterance }
    }

    private fun getCallbackInput(
        callback: TCallbackDirective,
        semanticFrames: List<SemanticFrame>
    ): Input.Callback {
        val name = callback.name
        val rawPayload = callback.payload
        if (!name2DirectiveClass.containsKey(name)) {
            logger.error("Unknown callback: {}", callback)

            throw AliceHandledException("Unknown callback", unknownCallbackAction, "Произошла ошибка")
        }
        return Input.Callback(
            directive = protoUtil.structToObject(rawPayload, name2DirectiveClass[name]!!),
            semanticFrames = semanticFrames
        )
    }

    @Suppress("UNCHECKED_CAST")
    private fun getClass(className: String): Class<CallbackDirective> {
        return try {
            val clazz = (Class.forName(className) ?: throw RuntimeException("Cant find class $className"))
            clazz as Class<CallbackDirective>
        } catch (e: ClassNotFoundException) {
            throw RuntimeException(e)
        }
    }

    companion object {
        private val logger = LogManager.getLogger(InputConverter::class.java)
    }
}
