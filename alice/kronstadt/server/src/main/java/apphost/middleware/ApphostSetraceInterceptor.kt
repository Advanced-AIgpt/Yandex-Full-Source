package ru.yandex.alice.kronstadt.server.apphost.middleware

import org.apache.logging.log4j.LogManager
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.alice.paskills.common.logging.protoseq.LogLevels
import ru.yandex.alice.paskills.common.logging.protoseq.Setrace
import ru.yandex.alice.paskills.common.logging.protoseq.SetraceEventLogger
import ru.yandex.web.apphost.api.request.RequestContext

internal data class ApphostParams(
    val reqid: String?,
)

@Component
@Order(4)
class ApphostSetraceInterceptor(private val setrace: Setrace) :
    ApphostRequestInterceptor {

    private val logger = LogManager.getLogger()

    companion object {
        private val ACTIVATION_STARTED_LOGGER = SetraceEventLogger(LogLevels.ACTIVATION_STARTED)
        private val ACTIVATION_FINISHED_LOGGER = SetraceEventLogger(LogLevels.ACTIVATION_FINISHED)
    }

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        val apphostParams = parseApphostParams(request)
        if (apphostParams?.reqid != null) {
            val rtLogToken = "${apphostParams.reqid}-${request.ruid}"
            logger.debug("Rtlog token: {}", rtLogToken)

            setrace.setupThreadContext(rtLogToken)
            ACTIVATION_STARTED_LOGGER.log()
        } else {
            logger.warn("Setrace setup context failed: missing reqid")
        }
        return true
    }

    override fun afterCompletion(
        handlingContext: ApphostRequestHandlingContext, request: RequestContext, ex: Exception?
    ) {
        ACTIVATION_FINISHED_LOGGER.log()
        setrace.clearThreadContext()
    }

    private fun parseApphostParams(context: RequestContext): ApphostParams? =
        context.getRequestItems("app_host_params").map {
            it.getJsonData(ApphostParams::class.java)
        }.firstOrNull()
}
