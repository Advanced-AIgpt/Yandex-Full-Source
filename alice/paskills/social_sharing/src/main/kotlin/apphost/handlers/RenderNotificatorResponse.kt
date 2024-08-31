package ru.yandex.alice.social.sharing.apphost.handlers

import NMatrix.NApi.Delivery
import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.web.apphost.api.request.RequestContext

@Component
class RenderNotificatorResponse(
    objectMapper: ObjectMapper,
): ApphostHandler {
    override val path = "/send_to_device/render_notificator_response"

    override fun handle(context: RequestContext) {
        val httpResponse = context
            .getSingleRequestItem(NOTIFICATOR_HTTP_RESPONSE)
            .getProtobufData(Http.THttpResponse.getDefaultInstance())
        logger.info("Notificator returned code {}")
        val notificatorResponse = Delivery.TDeliveryResponse.parseFrom(httpResponse.content.toByteArray())

        if (httpResponse.statusCode < 200 || httpResponse.statusCode >= 300) {
            logger.error("Bad response from notificator")
            return
        } else {
            logger.debug("Notificator response:\n{}\n{}",
                httpResponse.statusCode, httpResponse.content.toStringUtf8())
            context.addProtobufItem(NOTIFICATOR_SEND_PUSH_RESPONSE, notificatorResponse)
        }
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
