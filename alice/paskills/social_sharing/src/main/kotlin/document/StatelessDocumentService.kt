package ru.yandex.alice.social.sharing.document

import com.fasterxml.jackson.annotation.JsonCreator
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.Interface
import ru.yandex.alice.social.sharing.ProtoDeserializer
import ru.yandex.alice.social.sharing.ProtoSerializable
import ru.yandex.alice.social.sharing.UTF_8
import ru.yandex.alice.social.sharing.apphost.handlers.STATELESS_DOCUMENT_CONTEXT
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.web.apphost.api.request.RequestContext
import java.net.URLDecoder
import java.util.*
import java.util.Base64
import org.apache.commons.codec.binary.Base64 as ApacheCommonsBase64

interface StatelessDocumentService {
}

private fun String.trimToNull(): String? {
    return if (this != "") this else null
}

class InvalidUrlParamsException(
    reason: Exception?,
): Exception("Cannot deserialize ExternalSkillDocumentParams from url", reason)

object QueryParams {
    val SIGNATURE = "signature"
    val IMAGE_URL = "image_url"
}

data class ExternalSkillDocumentParams(
    val skillId: String,
    val imageUrl: String,
    val titleText: String,
    val subtitleText: String?,
    val buttonText: String?,
    val payload: String?,
    val signature: String,
    val requiredInterfaces: List<Interface>,
    val autostart: Boolean,
): ProtoSerializable<StatelessDocument.TExternalSkillUrlParams> {

    @JsonCreator
    constructor(
        @JsonProperty("skill_id") skillId: String,
        @JsonProperty("image_url") imageUrl: String,
        @JsonProperty("title_text") titleText: String,
        @JsonProperty("subtitle_text") subtitleText: String?,
        @JsonProperty("button_text") buttonText: String?,
        @JsonProperty("payload") payload: String?,
        @JsonProperty("signature") signature: String,
        @JsonProperty("required_interfaces") requiredInterfacesString: String?,
        @JsonProperty("autostart") autostart: Boolean,
    ): this(
        skillId = skillId,
        imageUrl = imageUrl,
        titleText = titleText,
        subtitleText = subtitleText,
        buttonText = buttonText,
        payload = payload,
        signature = signature,
        requiredInterfaces = requiredInterfacesString
            ?.split(",")?.map { Interface.fromStringUnsafe(it) }
            ?: emptyList(),
        autostart = autostart
    )

    @Throws(InvalidParamsException::class)
    fun validate() {
        if (skillId.isBlank()) throw MissingRequiredParameterException("skill_id")
        if (imageUrl.isBlank()) throw MissingRequiredParameterException("image_url")
        if (titleText.isBlank()) throw MissingRequiredParameterException("title_text")
        if (signature.isBlank()) throw MissingRequiredParameterException("signature")
        try {
            UUID.fromString(skillId)
        } catch (e: IllegalArgumentException) {
            throw InvalidUUIDException()
        }
    }

    override fun toProto(): StatelessDocument.TExternalSkillUrlParams {
        return StatelessDocument.TExternalSkillUrlParams.newBuilder()
            .setSkillId(skillId)
            .setImageUrl(imageUrl)
            .setTitleText(titleText)
            .setSubtitleText(subtitleText ?: "")
            .setButtonText(buttonText ?: "")
            .setPayload(payload ?: "")
            .setSignature(signature)
            .addAllRequiredInterfaces(requiredInterfaces.map { it.value })
            .setAutostart(autostart)
            .build()
    }

    companion object: ProtoDeserializer<StatelessDocument.TExternalSkillUrlParams, ExternalSkillDocumentParams> {
        private val logger = LogManager.getLogger()

        val EMPTY = ExternalSkillDocumentParams(
            skillId = "",
            imageUrl = "",
            titleText = "",
            subtitleText = "",
            buttonText = "",
            payload = "",
            signature = "",
            requiredInterfaces = emptyList(),
            autostart = false
        )

        override fun fromProto(proto: StatelessDocument.TExternalSkillUrlParams): ExternalSkillDocumentParams {
            return ExternalSkillDocumentParams(
                skillId = proto.skillId,
                imageUrl = proto.imageUrl,
                titleText = proto.titleText,
                subtitleText = proto.subtitleText.trimToNull(),
                buttonText = proto.buttonText.trimToNull(),
                payload = proto.payload.trimToNull(),
                signature = proto.signature,
                requiredInterfaces = proto.requiredInterfacesList.map { Interface.fromStringUnsafe(it) },
                autostart = proto.autostart
            )
        }
    }

}

data class StatelessDocumentContext(
    val urlParams: ExternalSkillDocumentParams
): ProtoSerializable<StatelessDocument.TStatelessDocumentContext> {

    override fun toProto(): StatelessDocument.TStatelessDocumentContext {
        return StatelessDocument.TStatelessDocumentContext.newBuilder()
            .setExternalSkillUrlParams(urlParams.toProto())
            .build()
    }

    companion object: ProtoDeserializer<StatelessDocument.TStatelessDocumentContext, StatelessDocumentContext> {

        @JvmStatic
        override fun fromProto(proto: StatelessDocument.TStatelessDocumentContext): StatelessDocumentContext {
            return StatelessDocumentContext(
                urlParams = ExternalSkillDocumentParams.fromProto(proto.externalSkillUrlParams),
            )
        }

        @JvmStatic
        fun fromContext(context: RequestContext, key: String = STATELESS_DOCUMENT_CONTEXT): StatelessDocumentContext {
            return fromProto(
                context
                    .getSingleRequestItem(key)
                    .getProtobufData(StatelessDocument.TStatelessDocumentContext.getDefaultInstance())
            )
        }
    }

}

class InvalidSignatureException(
    val expected: ByteArray,
    val got: ByteArray,
): Exception("Signatures do not match: expected ${Base64.getEncoder().encode(expected)}," +
    " got ${Base64.getEncoder().encode(got)}")

open class InvalidParamsException(
    message: String? = null,
    reason: Exception? = null,
): Exception(message, reason) {
    val code = "INVALID_REQUEST_PARAMS"
}

class MissingRequiredParameterException(
    parameter: String
): InvalidParamsException("Missing required parameter ${parameter}")

class InvalidUUIDException: InvalidParamsException()

@Component
class ExternalSkillStatelessDocumentService(
    private val objectMapper: ObjectMapper,
): StatelessDocumentService {

    private fun urlDecode(params: Map<String, String>): Map<String, String> {
        var decoded = params.mapValues { URLDecoder.decode(it.value, UTF_8) }
        if (!params.containsKey(QueryParams.SIGNATURE)) {
            return decoded
        }
        var iteration = 1
        while (iteration < URL_DECODE_MAX_ITERATIONS && !ApacheCommonsBase64.isBase64(decoded[QueryParams.SIGNATURE])) {
            decoded = decoded.mapValues { URLDecoder.decode(it.value, UTF_8) }
            iteration++
        }
        return decoded
    }

    @Throws(InvalidParamsException::class)
    fun parseParams(urlEncodedParams: Map<String, String>): ExternalSkillDocumentParams {
        val urlDecodedParams = urlDecode(urlEncodedParams)
        try {
            return objectMapper.convertValue(urlDecodedParams, ExternalSkillDocumentParams::class.java)
        } catch (e: IllegalArgumentException) {
            throw InvalidParamsException("Failed to deserialize request params", reason = e)
        }
    }

    companion object {
        @JvmStatic
        private val URL_DECODE_MAX_ITERATIONS = 2
    }
}
