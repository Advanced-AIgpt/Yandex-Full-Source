package ru.yandex.alice.kronstadt.server.http.middleware

import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpHeaders
import org.springframework.stereotype.Component
import org.springframework.web.servlet.HandlerInterceptor
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.server.http.X_OAUTH_TOKEN
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes
import java.util.UUID
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

private val sensitiveHeadersList = listOf(
    SecurityHeaders.SERVICE_TICKET_HEADER,
    SecurityHeaders.USER_TICKET_HEADER,
    SecurityHeaders.X_DEVELOPER_TRUSTED_TOKEN,
    SecurityHeaders.X_TRUSTED_SERVICE_TVM_CLIENT_ID,
    SecurityHeaders.X_UID,
    HttpHeaders.AUTHORIZATION,
    X_OAUTH_TOKEN
)

/**
 * Request Context is cleared in [RequestContextCleaningFilter]
 */
@Component
class RequestContextInterceptor(private val requestContext: RequestContext) : HandlerInterceptor {

    private val logger = LogManager.getLogger()

    override fun preHandle(request: HttpServletRequest, response: HttpServletResponse, handler: Any): Boolean {
        val requestId = getRequestId(request)
        requestContext.requestId = requestId
        requestContext.forwardedFor = request.getHeader("X-Forwarded-For")
        requestContext.userAgent = request.getHeader(HttpHeaders.USER_AGENT)

        request.getAttribute(SecurityRequestAttributes.UID_REQUEST_ATTR)?.let { uid ->
            requestContext.currentUserId = uid.toString()
        }

        (request.getAttribute(SecurityRequestAttributes.USER_TICKET_REQUEST_ATTR) as? String)?.let { userTicket ->
            requestContext.currentUserTicket = userTicket
        }
        (request.getAttribute(SecurityRequestAttributes.TVM_CLIENT_ID_REQUEST_ATTR) as? String?)?.let { tvmClientId ->
            requestContext.sourceTvmClientId = tvmClientId.toInt()
        }

        val headersText = StringBuilder()
        for (header in request.headerNames) {
            headersText
                .append(header)
                .append(": ")
                .append(
                    if (sensitiveHeadersList.contains(header)) {
                        "***"
                    } else {
                        request.getHeader(header)
                    }
                )
                .append("\n")
        }
        logger.info("Request headers:\n{}", headersText.toString())

        var authorizationHeader = request.getHeader(HttpHeaders.AUTHORIZATION)
        if (authorizationHeader == null) {
            authorizationHeader = request.getHeader(X_OAUTH_TOKEN)
        }
        requestContext.oauthToken = OAuthTokenParser.parse(authorizationHeader)
        return true
    }

    private fun getRequestId(request: HttpServletRequest): String {
        return request.getHeader("X-Request-Id")
            ?: request.getHeader("X-Req-Id")
            ?: UUID.randomUUID().toString()
    }
}
