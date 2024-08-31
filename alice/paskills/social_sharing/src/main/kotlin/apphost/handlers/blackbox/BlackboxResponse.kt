package ru.yandex.alice.social.sharing.apphost.handlers.blackbox

import NAppHostHttp.Http
import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.core.JsonParseException
import com.fasterxml.jackson.databind.JsonMappingException
import com.fasterxml.jackson.databind.ObjectMapper
import com.google.protobuf.ByteString
import org.apache.logging.log4j.LogManager
import ru.yandex.web.apphost.api.request.RequestContext
import java.io.IOException

data class BlackboxResponse(
    @JsonProperty("default_uid") val defaultUid: String?,
    @JsonProperty("user_ticket") val userTicket: String?,
) {

    companion object {
        private val logger = LogManager.getLogger()

        private fun isValidBBResponse(objectMapper: ObjectMapper, responseContent: ByteString): Boolean {
            try {
                val responseJson = objectMapper.readTree(responseContent.toStringUtf8())
                if (responseJson.isObject && responseJson.has("exception")) {
                    logger.error("Blackbox error: {}", responseContent)
                    return false
                } else {
                    return true
                }
            } catch (e: Exception) {
                return false
            }
        }

        @JvmStatic
        fun fromContext(
            context: RequestContext,
            objectMapper: ObjectMapper,
            key: String = "blackbox_http_response",
        ): BlackboxResponse? {
            val blackboxResponseO = context.getSingleRequestItemO(key)
            if (blackboxResponseO.isPresent) {
                val blackboxHttpResponse: Http.THttpResponse = blackboxResponseO.get()
                    .getProtobufData(Http.THttpResponse.getDefaultInstance())
                logger.debug("BB response content: {}", blackboxHttpResponse.content.toStringUtf8())
                if (!isValidBBResponse(objectMapper, blackboxHttpResponse.content)) {
                    return null
                }
                return try {
                    objectMapper.readValue(
                        blackboxHttpResponse.content.toByteArray(),
                        BlackboxResponse::class.java,
                    )
                } catch (e: Exception) {
                    when (e) {
                        is IOException, is JsonParseException, is JsonMappingException -> {
                            logger.error("Failed to parse blackbox response", e)
                            null;
                        }
                        else -> throw e
                    }
                }
            } else {
                return null;
            }
        }
    }

}
