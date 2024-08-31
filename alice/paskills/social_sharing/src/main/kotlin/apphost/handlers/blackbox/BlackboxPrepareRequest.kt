package ru.yandex.alice.social.sharing.apphost.handlers.blackbox

import NAppHostHttp.Http
import org.apache.logging.log4j.LogManager
import org.eclipse.jetty.server.CookieCutter
import org.springframework.stereotype.Component
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.social.sharing.ApphostHandler
import ru.yandex.alice.social.sharing.apphost.handlers.PROTO_HTTP_REQUEST
import ru.yandex.web.apphost.api.request.RequestContext

private data class AuthCookies(
    val sessionid: String?,
    val sessionid2: String?,
) {
    val isEmpty: Boolean
        get() = sessionid == null && sessionid2 == null

    companion object {
        val EMPTY = AuthCookies(null, null)
    }
}

@Component
class BlackboxPrepareRequest: ApphostHandler {

    override val path = "/blackbox/prepare_request"

    override fun handle(context: RequestContext) {
        val httpRequest = context
            .getSingleRequestItem(PROTO_HTTP_REQUEST)
            .getProtobufData(Http.THttpRequest.getDefaultInstance())
        val host = getHost(httpRequest)
        if (host == null) {
            logger.info("Do not create blackbox request: Host header is empty")
            return
        }
        val ip = getIp(httpRequest)
        if (ip == null) {
            logger.info("Do not create blackbox request: user ip is empty")
            return
        }
        val cookies = getCookies(httpRequest)
        if (cookies.isEmpty) {
            logger.info("Do not create blackbox request: cookies are empty")
            return
        }
        val uriComponents = UriComponentsBuilder.newInstance()
            .path("")
            .queryParam("method", "sessionid")
            .queryParam("host", host)
            .queryParam("userip", ip)
            .queryParam("get_user_ticket", "yes")
            .queryParam("multisession", "yes")
            .queryParam("format", "json")
        if (cookies.sessionid != null ) {
            uriComponents.queryParam("sessionid", cookies.sessionid)
        }
        if (cookies.sessionid2 != null) {
            uriComponents.queryParam("sslsessionid", cookies.sessionid2)
        }
        val blackboxRequest = Http.THttpRequest.newBuilder()
            .setMethod(Http.THttpRequest.EMethod.Get)
            .setPath(uriComponents.build().toString())
            .addHeaders(Http.THeader.newBuilder()
                .setName("Accept")
                .setValue("application/json")
                .build()
            )
            .build()
        logger.info("Blackbox request: {}", uriComponents.build().toString())
        context.addProtobufItem("blackbox_http_request", blackboxRequest)
    }

    private fun getHost(request: Http.THttpRequest): String? {
        val hostHeader = request.headersList.first { it.name.toLowerCase() == "host" }
        return hostHeader?.value
    }

    private fun getIp(request: Http.THttpRequest): String? {
        return request.headersList
            .firstOrNull {
                listOf("x-real-ip", "x-forwarded-for", "x-forwarded-for-y").contains(it.name.toLowerCase())
            }?.value
    }

    private fun getCookies(request: Http.THttpRequest): AuthCookies {
        val cookieHeaders = request.headersList.first { it.name.toLowerCase() == "cookie" }
        if (cookieHeaders == null) {
            return AuthCookies.EMPTY
        }
        val cookieCutter = CookieCutter()
        cookieCutter.addCookieField(cookieHeaders.value)
        val cookies = cookieCutter.cookies
        var sessionId: String? = null
        var sessionId2: String? = null
        for (cookie in cookies) {
            if (cookie.name == "Session_id") {
                sessionId = cookie.value
            } else if (cookie.name == "sessionid2") {
                sessionId2 = cookie.value
            }
        }
        return AuthCookies(sessionId, sessionId2)
    }

    companion object {
        private val logger = LogManager.getLogger()
    }

}
