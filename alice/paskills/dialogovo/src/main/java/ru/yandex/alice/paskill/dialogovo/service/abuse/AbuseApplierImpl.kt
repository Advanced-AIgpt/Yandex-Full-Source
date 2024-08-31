package ru.yandex.alice.paskill.dialogovo.service.abuse

import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.domain.Censored
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse
import ru.yandex.alice.paskill.dialogovo.utils.AnnotationWalker
import ru.yandex.alice.paskill.dialogovo.utils.ValuePathWrapper

private const val SUBSTITUTE_TEXT_MARKER = "<censored>"
private const val SUBSTITUTE_TTS_MARKER = "<speaker audio=\"beep_mat_1.opus\">"
private val CENSORED_REGEX = "<censored>.*?</censored>".toRegex()

@Component
internal open class AbuseApplierImpl(private val service: AbuseService) : AbuseApplier {

    private val walker = AnnotationWalker(Censored::class.java)

    private val logger = LogManager.getLogger()

    override fun apply(response: WebhookResponse) {
        try {
            val items = walker.traverseObject(response)
                .map { pathWrapper -> CensoredValuePathWrapper(pathWrapper) }

            if (items.isEmpty()) {
                return
            }

            val verdicts: Map<String, String> = service.checkAbuse(items)
            for (item in items) {
                verdicts[item.id]?.replace(
                    CENSORED_REGEX, if (item.isVoice) SUBSTITUTE_TTS_MARKER else SUBSTITUTE_TEXT_MARKER
                )?.also { item.apply(it) }
            }
        } catch (e: IllegalAccessException) {
            logger.error("Unable to substitute censored fields", e)
        }
    }

    private class CensoredValuePathWrapper(private val pathWrapper: ValuePathWrapper<Censored>) : AbuseDocument {
        val isVoice: Boolean
            get() = pathWrapper.annotation.isVoice
        override val id: String
            get() = pathWrapper.path
        override val value: String
            get() = pathWrapper.value

        @Throws(IllegalAccessException::class)
        fun apply(newValue: String) {
            pathWrapper.apply(newValue)
        }
    }
}
