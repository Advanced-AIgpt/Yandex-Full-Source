package ru.yandex.alice.paskill.dialogovo.utils

import com.fasterxml.jackson.core.JsonParser
import com.fasterxml.jackson.databind.DeserializationContext
import com.fasterxml.jackson.databind.JsonDeserializer
import com.fasterxml.jackson.databind.JsonNode
import org.hibernate.validator.messageinterpolation.ResourceBundleMessageInterpolator
import org.slf4j.LoggerFactory
import org.springframework.stereotype.Component
import ru.yandex.alice.paskill.dialogovo.external.v1.response.TeaserMeta
import java.util.Locale
import javax.validation.MessageInterpolator
import javax.validation.Validation

@Component
open class TeasersMetaDeserializer : JsonDeserializer<List<TeaserMeta>?>() {
    override fun deserialize(p: JsonParser, ctxt: DeserializationContext?): List<TeaserMeta>? {
        val codec = p.codec
        val node = codec.readTree<JsonNode>(p)
        if (node.isArray) {
            val teasersMeta: ArrayList<TeaserMeta> = ArrayList()
            val iterator = node.iterator()
            while (iterator.hasNext()) {
                val element = iterator.next()
                try {
                    val teaserMeta = codec.treeToValue(element, TeaserMeta::class.java)
                    val constraintViolations = validator.validate(teaserMeta)
                    if (constraintViolations.isEmpty()) {
                        teasersMeta.add(teaserMeta)
                    } else {
                        logger.error("Error while validating teaser ${constraintViolations.map { it.message }}")
                    }
                } catch (e: Exception) {
                    logger.error("Error while parsing teaser ${e.message}", e)
                }
            }
            return teasersMeta
        } else {
            logger.error("Failed to parse TeaserMeta: expected Array, got ${node.nodeType}")
            return null
        }
    }

    companion object {
        private val logger = LoggerFactory.getLogger(TeasersMetaDeserializer::class.java)

        private val validator = Validation.byDefaultProvider().configure()
            .messageInterpolator(object : ResourceBundleMessageInterpolator() {
                override fun interpolate(message: String, context: MessageInterpolator.Context): String {
                    return super.interpolate(message, context, Locale("ru", "RU"))
                }
            }).buildValidatorFactory().validator
    }
}
