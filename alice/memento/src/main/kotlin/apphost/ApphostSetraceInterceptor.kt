package ru.yandex.alice.memento.apphost

import org.apache.logging.log4j.LogManager
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.alice.paskills.common.logging.protoseq.LogLevels
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace
import ru.yandex.alice.paskills.common.logging.protoseq.SetraceEventLogger
import ru.yandex.web.apphost.api.request.RequestContext

@Component
@Order(3)
class ApphostSetraceInterceptor(private val setrace: Setrace) : ApphostRequestInterceptor {

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        val apphostParams = parseApphostParams(request)
        if (apphostParams?.reqid != null) {
            val rtLogToken = "${apphostParams.reqid}-${request.ruid}"
            logger.debug("Rtlog token: {}", rtLogToken)

            handlingContext.setAttribute(RTLOG_TOKEN_ATTRIBUTE, rtLogToken)
            setrace.setupThreadContext(rtLogToken)
            ACTIVATION_STARTED_LOGGER.log()
        } else {
            logger.warn("Setrace setup context failed: missing reqid")
        }
        return true;
    }

    override fun afterCompletion(
        handlingContext: ApphostRequestHandlingContext,
        request: RequestContext,
        ex: Exception?
    ) {
        if (handlingContext.getAttribute(RTLOG_TOKEN_ATTRIBUTE) != null) {
            ACTIVATION_FINISHED_LOGGER.log()
            setrace.clearThreadContext()
        }
    }

    private fun parseApphostParams(context: RequestContext): ApphostParams? =
        context.getSingleRequestItemO("app_host_params")
            .map { it.getJsonData(ApphostParams::class.java) }
            .orElse(null)

    private data class ApphostParams(val reqid: String?)

    companion object {
        private val logger = LogManager.getLogger(ApphostSolomonInterceptor::class.java)
        private val RTLOG_TOKEN_ATTRIBUTE = ApphostSetraceInterceptor::class.java.name + ".RtLogToken"
        private val ACTIVATION_STARTED_LOGGER = SetraceEventLogger(LogLevels.ACTIVATION_STARTED)
        private val ACTIVATION_FINISHED_LOGGER = SetraceEventLogger(LogLevels.ACTIVATION_FINISHED)
    }
}
