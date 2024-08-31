package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NAppHostHttp.Http
import com.fasterxml.jackson.core.type.TypeReference
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.commons.codec.binary.Base64
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.*
import ru.yandex.alice.social.sharing.apphost.handlers.*
import ru.yandex.alice.social.sharing.document.ExternalSkillDocumentParams
import ru.yandex.alice.social.sharing.document.ExternalSkillStatelessDocumentService
import ru.yandex.alice.social.sharing.document.InvalidParamsException
import ru.yandex.alice.social.sharing.document.QueryParams
import ru.yandex.alice.social.sharing.proto.WebApi
import ru.yandex.alice.social.sharing.proto.context.ListDevicesProto
import ru.yandex.alice.social.sharing.proto.context.StatelessDocument
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.web.apphost.api.request.RequestContext
import java.net.URLDecoder
import java.util.*

abstract class ValidateStatelessDocumentRequiredParams(
    protected val statelessDocumentService: ExternalSkillStatelessDocumentService,
    protected val objectMapper: ObjectMapper,
    metricRegistry: MetricRegistry,
): ApphostHandler {

    protected val doubleUrlEncodeCounter = metricRegistry.counter("double_urlencode")

    abstract fun parseDocumentParams(request: Http.THttpRequest): TreeMap<String, String>

    override fun handle(context: RequestContext) {
        val httpRequest = context
            .getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        val urlEncodedParams: TreeMap<String, String>
        val params: ExternalSkillDocumentParams
        try {
            urlEncodedParams = parseDocumentParams(httpRequest)
            if (urlEncodedParams.containsKey(QueryParams.SIGNATURE) &&
                !Base64.isBase64(URLDecoder.decode(urlEncodedParams[QueryParams.SIGNATURE], UTF_8))
            ) {
                // In some cases iOS can do additional urlencoding before the URL is opened in Safari, for example
                // "title_text=Заголовок&subtitle_text=Подзаголовок&image_url=https%3A%2F%2Fyastatic.net%2Fs3%2Fhome-static%2F_%2F6%2FL%2FsRWLDRTog6jt1kgf7Kg3BQ71g.svg&signature=GYHTRpU9hvGz/wokO56vwnyQgmEDHIJC8t7eWs%2BPmmA%3D&skill_id=689f64c4-3134-42ba-8685-2b7cd8f06f4d"
                // will be transformed to
                // "title_text=Заголовок&subtitle_text=Подзаголовок&image_url=https%253A%252F%252Fyastatic.net%252Fs3%252Fhome-static%252F_%252F6%252FL%252FsRWLDRTog6jt1kgf7Kg3BQ71g.svg&signature=GYHTRpU9hvGz/wokO56vwnyQgmEDHIJC8t7eWs%252BPmmA%253D&skill_id=689f64c4-3134-42ba-8685-2b7cd8f06f4d"
                // when opened in an iOS app.
                // To fix that, we do urldecoding multiple times until we get a valid base64 string in signature field.
                // Note that not all fields are converted multiple times, title_text and subtitle_text were left as they were.
                // See PASKILLS-7411 for details
                logger.warn("Found double urlencoded signature: {}", urlEncodedParams[QueryParams.SIGNATURE])
                doubleUrlEncodeCounter.inc()
                urlEncodedParams[QueryParams.SIGNATURE] = URLDecoder.decode(urlEncodedParams[QueryParams.SIGNATURE], UTF_8)
                urlEncodedParams[QueryParams.IMAGE_URL] = URLDecoder.decode(urlEncodedParams[QueryParams.IMAGE_URL], UTF_8)
            }
            params = statelessDocumentService.parseParams(urlEncodedParams)
            params.validate()
        } catch (e: InvalidParamsException) {
            logger.warn("Failed to parse all required parameters from query", e)
            context.addProtobufItem(
                WEB_PAGE_DATA,
                WebApi.TWebPageTemplateData.newBuilder()
                    .setError(
                        WebApi.TWebPageTemplateData.TError.newBuilder().setCode(e.code)
                    )
                    .build()
            )
            return
        }
        val documentContext: StatelessDocument.TStatelessDocumentContext =
            StatelessDocument.TStatelessDocumentContext.newBuilder()
                .setExternalSkillUrlParams(params.toProto())
                .build()
        context.addProtobufItem(STATELESS_DOCUMENT_CONTEXT, documentContext)
        val allowedKeys: Set<String> = objectMapper
            .convertValue(params, object: TypeReference<Map<String, Any>>(){})
            .keys
        val signatureValidationParams: StatelessDocument.TSignatureValidationParams =
            StatelessDocument.TSignatureValidationParams.newBuilder()
                .putAllUrlEncodedParams(
                    urlEncodedParams.filter { allowedKeys.contains(it.key) }
                )
                .build()
        if (params.autostart) {
            context.addFlag(FLAG_AUTOSTART)
        }
        context.addProtobufItem(SIGNATURE_VALIDATION_PARAMS, signatureValidationParams)
        context.addProtobufItem("get_skill_info_http_request", prepareGetSkillRequest(params.skillId))
        context.addProtobufItem("get_skill_secret_http_request", prepareGetSkillSecretRequest(params.skillId))

        val requiredFeatures = params.requiredInterfaces
            .flatMap { INTERFACE_TO_REQUIRED_FEATURES[it] ?: emptyList() }
            .toSet()
        context.addProtobufItem(
            LIST_DEVICES_REQUEST,
            ListDevicesProto.TListDevicesRequest.newBuilder()
                .addAllRequiredFeatures(requiredFeatures)
                .build()
        )
        if (requiredFeatures.contains(ClientFeatures.AUDIO_CLIENT)) {
            context.addFlag(FLAG_REQUIRE_AUTH)
        }
    }

    protected fun prepareGetSkillRequest(skillId: String): Http.THttpRequest {
        return Http.THttpRequest.newBuilder()
            .setMethod(Http.THttpRequest.EMethod.Get)
            .setPath("/api/external/v2/skills/$skillId/")
            .build()
    }

    protected fun prepareGetSkillSecretRequest(skillId: String): Http.THttpRequest {
        return Http.THttpRequest.newBuilder()
            .setMethod(Http.THttpRequest.EMethod.Get)
            .setPath("/api/external/v2/skills/$skillId/hmac-secrets")
            .build()
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

@Component
class ValidateStatelessDocumentRequiredParamsFromPath(
    statelessDocumentService: ExternalSkillStatelessDocumentService,
    objectMapper: ObjectMapper,
    metricRegistry: MetricRegistry,
) : ValidateStatelessDocumentRequiredParams(statelessDocumentService, objectMapper, metricRegistry) {

    override val path = "/stateless_document/validate_required_params_from_path"

    override fun parseDocumentParams(request: Http.THttpRequest): TreeMap<String, String> {
        val queryString = getQueryString(request.path)
        return splitQueryString(queryString)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}

@Component
class ValidateStatelessDocumentRequiredParamsFromReferer(
    statelessDocumentService: ExternalSkillStatelessDocumentService,
    objectMapper: ObjectMapper,
    metricRegistry: MetricRegistry,
) : ValidateStatelessDocumentRequiredParams(statelessDocumentService, objectMapper, metricRegistry) {

    override val path = "/stateless_document/validate_required_params_from_referer"

    override fun parseDocumentParams(request: Http.THttpRequest): TreeMap<String, String> {
        val refererHeader = request.headersList
            .firstOrNull() { h -> h.name.lowercase().equals(REFERER.lowercase()) }
        if (refererHeader == null) {
            logger.error("Failed to find referer header")
            throw InvalidParamsException()
        }
        val referer = refererHeader.value
        val queryString = getQueryString(referer)
        return splitQueryString(queryString)
    }

    companion object {
        @JvmStatic
        private val REFERER = "Referer"

        private val logger = LogManager.getLogger()
    }

}
