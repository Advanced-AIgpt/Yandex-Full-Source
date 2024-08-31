package ru.yandex.alice.social.sharing.apphost.handlers.document.stateless

import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Value
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_REQUEST
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_RESPONSE
import ru.yandex.alice.social.sharing.apphost.handlers.blackbox.BlackboxResponse
import ru.yandex.web.apphost.api.request.RequestContext

// balancer should be configured to set these headers so that we'd know
// initial request hostname and url before request rewrites
private val HEADER_X_YANDEX_SCHEME = "X-Yandex-Scheme".lowercase()
private val HEADER_X_YANDEX_HOST = "X-Yandex-Host".lowercase()
private val HEADER_X_YANDEX_URL = "X-Yandex-Url".lowercase()

@Component
class RedirectToPassport(
    private val objectMapper: ObjectMapper,
    @Value("\${passport.url}") private val passportUrl: String,
    @Value("\${passport.retpath.default}") private val defaultRetPath: String,
): ApphostHandler {

    override val path = "/stateless_document/redirect_to_passport"

    private fun makeRetPath(httpRequest: Http.THttpRequest): String {
        var host: String? = null
        var url: String? = null
        var scheme: String? = "https"
        for (header in httpRequest.headersList) {
            if (header.name.lowercase() == HEADER_X_YANDEX_SCHEME) {
                scheme = header.value
            } else if (header.name.lowercase() == HEADER_X_YANDEX_HOST) {
                host = header.value
            } else if (header.name.lowercase() == HEADER_X_YANDEX_URL) {
                url = header.value
            }
        }
        if (url != null) {
            // hotfix for retpath parameter until BALANCERSUPPORT-3443 is not resolved
            url = url.replace("/apphost/social_sharing/document_from_request", "/sharing/doc")
        }
        if (scheme != null && host != null && url != null) {
            return "$scheme://$host$url"
        } else {
            val pathParts = httpRequest.path.split("\\?".toRegex(), 2)
            val path = pathParts[0]
            val retpathBuilder: UriComponentsBuilder = UriComponentsBuilder.fromUriString(defaultRetPath)
                .scheme(if (httpRequest.scheme == Http.THttpRequest.EScheme.Https) "https" else "http")
                .path(path)
            val query = if (pathParts.size > 1) pathParts[1] else null
            val retPath = retpathBuilder.toUriString() + "?" + (query ?: "")
            return retPath
        }
    }

    override fun handle(context: RequestContext) {
        val blackboxResponse = BlackboxResponse.fromContext(context, objectMapper)
        if (blackboxResponse == null ||
            blackboxResponse.defaultUid.isNullOrBlank() ||
            blackboxResponse.defaultUid == UNAUTHORIZED_UID
        ) {
            logger.info("User is not authorized, redirecting to passport")
            val httpRequest = context.getSingleRequestItem(PROTO_HTTP_REQUEST)
                .getProtobufData(Http.THttpRequest.getDefaultInstance())
            val uri = UriComponentsBuilder.fromUriString(passportUrl)
                .queryParam("origin", "alice_social_sharing")
                .queryParam("retpath", makeRetPath(httpRequest))
            val response: Http.THttpResponse = Http.THttpResponse.newBuilder()
                .setStatusCode(302)
                .addHeaders(
                    Http.THeader.newBuilder()
                        .setName("Location")
                        .setValue(uri.toUriString())
                        .build()
                )
                .build()
            context.addProtobufItem(PROTO_HTTP_RESPONSE, response)
        } else {
            logger.info("User is authorized, will not redirect to passport")
        }
    }

    companion object {
        private val logger = LogManager.getLogger()

        @JvmStatic
        private val UNAUTHORIZED_UID = "0"
    }

}
