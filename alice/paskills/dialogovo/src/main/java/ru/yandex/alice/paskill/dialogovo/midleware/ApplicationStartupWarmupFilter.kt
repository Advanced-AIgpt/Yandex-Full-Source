package ru.yandex.alice.paskill.dialogovo.midleware

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import java.util.concurrent.atomic.AtomicBoolean
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@Order(15)
@Component
class ApplicationStartupWarmupFilter : Filter {
    val ready = AtomicBoolean(false)

    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        if (ready.get()) {
            chain.doFilter(request, response)
        } else if (request is HttpServletRequest && response is HttpServletResponse) {
            if (request.getHeader(WARMUP_HEADER) == "true" || request.servletPath.contains("solomon")) {
                chain.doFilter(request, response)
            } else {
                response.sendError(503, "service not ready")
            }
        } else {
            chain.doFilter(request, response)
        }
    }

    companion object {
        internal const val WARMUP_HEADER = "X-Ya-Dialogovo-Warmup"
    }
}
