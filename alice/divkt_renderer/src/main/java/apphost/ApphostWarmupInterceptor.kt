package ru.yandex.alice.divktrenderer.apphost

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.web.apphost.api.exception.AppHostFastException
import ru.yandex.web.apphost.api.request.RequestContext
import java.util.concurrent.atomic.AtomicBoolean

@Component
@Order(10)
class ApphostWarmupInterceptor : ApphostRequestInterceptor {

    val ready = AtomicBoolean(false)

    companion object {
        const val WARMUP_FLAG = "divkt_renderer_warmup"
    }

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        return if (ready.get()) {
            true
        } else {
            if (request.checkFlag(WARMUP_FLAG)) {
                true
            } else {
                throw AppHostFastException("warmup in progress", false)
            }
        }
    }
}
