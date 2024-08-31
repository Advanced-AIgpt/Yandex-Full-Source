package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.HmacSignature
import ru.yandex.alice.social.sharing.UTF_8
import ru.yandex.alice.social.sharing.apphost.handlers.SIGNATURE_VALIDATION_OK
import ru.yandex.alice.social.sharing.apphost.handlers.SIGNATURE_VALIDATION_PARAMS
import ru.yandex.alice.social.sharing.apphost.handlers.WEB_PAGE_DATA
import ru.yandex.alice.social.sharing.document.QueryParams
import ru.yandex.alice.social.sharing.makeErrorSensor
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.RequestContext
import java.net.URLDecoder
import java.util.*

class InvalidSignatureException(msg: String): Exception("Signature is invalid: $msg")

class ValidateStatelessDocumentSignature(
    private val objectMapper: ObjectMapper,
    metricRegistry: MetricRegistry,
) : ApphostHandler {

    override val path = "/stateless_document/validate_signature"

    private val invalidSignatureCounter = makeErrorSensor(metricRegistry, "invalid_signature")
    private val invalidBase64Counter = makeErrorSensor(metricRegistry, "signature_invalid_base_64")
    private val signatureMissingCounter = makeErrorSensor(metricRegistry, "signature_missing")

    private fun parseSignatureFromParams(params: TreeMap<String, String>): ByteArray {
        val signatureFromParamsBase64 = URLDecoder.decode(params[QueryParams.SIGNATURE], UTF_8)
        if (signatureFromParamsBase64.isNullOrEmpty()) {
            signatureMissingCounter.inc()
            throw InvalidSignatureException("request doesn't have signature parameter")
        }
        return try {
            Base64.getDecoder().decode(signatureFromParamsBase64)
        } catch (e: IllegalArgumentException) {
            invalidBase64Counter.inc()
            throw InvalidSignatureException("invalid base64")
        }
    }

    override fun handle(context: RequestContext) {
        val signatureValidationParamsProto = context.getSingleRequestItem(SIGNATURE_VALIDATION_PARAMS)
            .getProtobufData(StatelessDocument.TSignatureValidationParams.getDefaultInstance())
        val parameterSets: List<TreeMap<String, String>> = generatePossibleParameterEncodings(
            signatureValidationParamsProto.urlEncodedParamsMap)
        val skillSecretHttpResponse: Http.THttpResponse = context
            .getSingleRequestItem("get_skill_secret_http_response")
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        val skillSecretResponse: SkillSecretResponse = objectMapper.readValue(
            skillSecretHttpResponse.content.toStringUtf8(),
            SkillSecretResponse::class.java,
        )
        val secrets = skillSecretResponse.secrets.map { it.value }
        val signatureIsValid = validateSignature(parameterSets, secrets)
        if (signatureIsValid) {
            context.addProtobufItem(
                SIGNATURE_VALIDATION_OK,
                StatelessDocument.TSignatureValidationOk.newBuilder().build()
            )
        } else {
            logger.warn("Failed to validate request signature")
            invalidSignatureCounter.inc()
            context.addProtobufItem(
                WEB_PAGE_DATA,
                WebApi.TWebPageTemplateData.newBuilder()
                    .setError(
                        WebApi.TWebPageTemplateData.TError.newBuilder()
                            .setCode("INVALID_SIGNATURE")
                    )
                    .build()
            )
        }
    }

    fun generatePossibleParameterEncodings(params: Map<String, String>): List<TreeMap<String, String>> {
        // urlencoding can be done in multiple ways, for example, some browsers and messengers
        // replace "+" with "%20". To validate signature, we can generate all possible parameter values
        val encodedWithPlus = TreeMap(params.mapValues { it.value.replace("%20", "+") })
        return listOf(TreeMap(params), encodedWithPlus)
    }

    fun validateSignature(parameterSets: List<TreeMap<String, String>>, secrets: List<String>): Boolean {
        for (params in parameterSets) {
            for (secret in secrets) {
                val signature = parseSignatureFromParams(params)
                val signatureIsValid = HmacSignature.validate(params, secret, signature)
                if (signatureIsValid) {
                    return true
                }
            }
        }
        return false
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}

@Configuration
open class SignatureValidatorConfiguration {
    @Bean("validateStatelessDocumentSignature")
    open fun validateStatelessDocumentSignature(
        objectMapper: ObjectMapper,
        metricRegistry: MetricRegistry,
    ): ValidateStatelessDocumentSignature {
        return ValidateStatelessDocumentSignature(
            objectMapper,
            metricRegistry,
        )
    }
}
