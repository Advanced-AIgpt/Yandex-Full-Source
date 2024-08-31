package ru.yandex.alice.kronstadt.server.apphost.middleware

import org.slf4j.LoggerFactory
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.web.apphost.api.request.RequestContext
import java.time.Duration
import java.time.Instant

@Component
@Order(1)
class ApphostAccessLogInterceptor(private val requestContext: ru.yandex.alice.kronstadt.core.RequestContext) :
    ApphostRequestInterceptor {
    private val startTimeAttribute = ApphostAccessLogInterceptor::class.java.name + ".RequestStartTime"
    private val logger = LoggerFactory.getLogger("ACCESS_LOG")
    private val UNDEFINED = "-"

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        handlingContext.setAttribute(startTimeAttribute, Instant.now())
        return true
    }

    override fun afterCompletion(
        handlingContext: ApphostRequestHandlingContext,
        request: RequestContext,
        ex: Exception?
    ) {
        val requestDuration = handlingContext.getAttribute(startTimeAttribute)
            ?.let { it as Instant }
            ?.let { startTime ->
                millisecondsToSecondsString(Duration.between(startTime, Instant.now()).toMillis())
            }
            ?: UNDEFINED

        // log format for compatiblity with existing
        logger.info(
            "{} {} {} \"{} {} {}\" {} \"{}\" \"{}\" {} {} {} {} {} {} {} \"{}\" {} {} {} \"{}\" {}",
            UNDEFINED, // $http_host
            UNDEFINED, // $remote_addr
            requestContext.requestId, // $request_id
            UNDEFINED, // "$request_method"
            handlingContext.path, // "$request_path"
            "rpc", // "$request_protocol"
            UNDEFINED, // $status
            UNDEFINED, // "$http_referer"
            UNDEFINED, // "$http_user_agent"
            requestContext.sourceTvmClientId, // $tvm_client_id
            requestDuration, // $request_time
            requestContext.currentUserId, // $uid from tvm user ticket
            UNDEFINED, // "$mm_request.dialogId"
            UNDEFINED, // "$mm_request.appId"
            UNDEFINED, // "$mm_request.uuid"
            UNDEFINED, // "$mm_request.deviceId"
            UNDEFINED, // "$mm_request.platform"
            UNDEFINED, // "$mm_request.voiceSession"
            UNDEFINED, // "$mm_request.currentSkillId"
            UNDEFINED, // "$mm_request.sessionId"
            UNDEFINED, // "$forwarders",
            UNDEFINED // $raw_remote_addr
        )
    }

    private fun millisecondsToSecondsString(millis: Long): String =
        "${millis / 1000}.${String.format("%03d", millis % 1000)}"
}
