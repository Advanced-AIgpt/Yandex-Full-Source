package ru.yandex.alice.paskill.dialogovo.midleware

import org.slf4j.LoggerFactory
import org.springframework.http.HttpStatus
import org.springframework.http.converter.HttpMessageNotReadableException
import org.springframework.stereotype.Component
import org.springframework.web.HttpRequestMethodNotSupportedException
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext.MegaMindRequestContext
import ru.yandex.alice.paskill.dialogovo.utils.Headers
import java.time.Duration
import java.time.Instant
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

private const val UNDEFINED = "-"

@Component
open class AccessLogger(
    private val requestContext: RequestContext,
    private val dialogovoRequestContext: DialogovoRequestContext
) {
    private val logger = LoggerFactory.getLogger("ACCESS_LOG")

    private val exceptionClassToStatusMap = mapOf(
        HttpRequestMethodNotSupportedException::class.java to HttpStatus.METHOD_NOT_ALLOWED,
        HttpMessageNotReadableException::class.java to HttpStatus.BAD_REQUEST
    )

    fun log(request: HttpServletRequest, response: HttpServletResponse, ex: Exception?) {
        val status = ex?.let { exceptionClassToStatusMap[it.javaClass] }?.value() ?: response.status

        val agent = requestContext.userAgent ?: UNDEFINED
        val requestId = requestContext.requestId ?: getRequestId(request)
        val tvmClientId = requestContext.sourceTvmClientId?.toString() ?: UNDEFINED

        val sourceIp = getSourceIpAddress(request)
        val uid = requestContext.currentUserId ?: UNDEFINED
        val referer = request.getHeader(Headers.REFERER) ?: UNDEFINED
        val requestDuration = request.getAttribute(AccessLogFilter.ACCESS_LOG_START_TIME_ATTRIBUTE)
            ?.let { it as Instant }
            ?.let { startTime ->
                millisecondsToSecondsString(Duration.between(startTime, Instant.now()).toMillis())
            }
            ?: UNDEFINED

        val forwarders = requestContext.forwardedFor
            ?: request.getHeader(Headers.X_FORWARDED_FOR)
            ?: UNDEFINED
        val mmContextO: MegaMindRequestContext? = dialogovoRequestContext.megaMindRequestContext
        val dialogId = mmContextO?.dialogId ?: UNDEFINED
        val appId = mmContextO?.appId ?: UNDEFINED
        val uuid = mmContextO?.uuid ?: UNDEFINED
        val deviceId = mmContextO?.deviceId ?: UNDEFINED
        val platform = mmContextO?.platform ?: UNDEFINED
        val voiceSession = mmContextO?.voiceSession?.toString() ?: UNDEFINED
        val currentSkillId = mmContextO?.currentSkillId ?: UNDEFINED
        val sessionId = mmContextO?.sessionId ?: UNDEFINED
        logger.trace(
            "{} {} {} \"{} {} {}\" {} \"{}\" \"{}\" {} {} {} {} {} {} {} \"{}\" {} {} {} \"{}\" {}",
            request.serverName, // $http_host
            sourceIp, // $remote_addr
            requestId, // $request_id
            request.method, // "$request_method"
            getRequestURI(request), // "$request_path"
            request.protocol, // "$request_protocol"
            status, // $status
            referer, // "$http_referer"
            agent, // "$http_user_agent"
            tvmClientId, // $tvm_client_id
            requestDuration, // $request_time
            uid, // $uid from tvm user ticket
            dialogId, // "$mm_request.dialogId"
            appId, // "$mm_request.appId"
            uuid, // "$mm_request.uuid"
            deviceId, // "$mm_request.deviceId"
            platform, // "$mm_request.platform"
            voiceSession, // "$mm_request.voiceSession"
            currentSkillId, // "$mm_request.currentSkillId"
            sessionId, // "$mm_request.sessionId"
            forwarders, // "$forwarders",
            request.remoteAddr // $raw_remote_addr
        )
    }

    private fun millisecondsToSecondsString(millis: Long): String =
        "${millis / 1000}.${String.format("%03d", millis % 1000)}"

    private fun getSourceIpAddress(request: HttpServletRequest): String {
        return request.getHeader(Headers.X_REAL_IP) ?: request.remoteAddr
    }

    private fun getRequestURI(request: HttpServletRequest?): String = request?.requestURI ?: UNDEFINED

    private fun getRequestId(request: HttpServletRequest): String =
        request.getHeader(Headers.X_REQUEST_ID) ?: request.getHeader(Headers.X_REQ_ID) ?: UNDEFINED
}
