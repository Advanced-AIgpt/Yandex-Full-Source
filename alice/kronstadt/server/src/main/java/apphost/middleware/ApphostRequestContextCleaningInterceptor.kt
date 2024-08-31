package ru.yandex.alice.kronstadt.server.apphost.middleware

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.log.LoggingContextHolder
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestHandlingContext
import ru.yandex.alice.paskills.common.apphost.spring.ApphostRequestInterceptor
import ru.yandex.web.apphost.api.request.RequestContext

@Component
@Order(0)
class ApphostRequestContextCleaningInterceptor(private val requestContext: ru.yandex.alice.kronstadt.core.RequestContext) :
    ApphostRequestInterceptor {

    override fun preHandle(handlingContext: ApphostRequestHandlingContext, request: RequestContext): Boolean {
        LoggingContextHolder.Companion.clearCurrent()
        requestContext.clear()
        return true
    }

    override fun afterCompletion(
        handlingContext: ApphostRequestHandlingContext,
        request: RequestContext,
        ex: Exception?
    ) {
        LoggingContextHolder.Companion.clearCurrent()
        requestContext.clear()
    }
}
