package ru.yandex.alice.paskill.dialogovo.midleware

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import java.time.Instant
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@Order(5)
@Component
class AccessLogFilter(private val accessLogger: AccessLogger) : Filter {
    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        request.setAttribute(ACCESS_LOG_START_TIME_ATTRIBUTE, Instant.now())
        var ex: Exception? = null
        try {
            chain.doFilter(request, response)
        } catch (e: Exception) {
            ex = e
        } finally {
            accessLogger.log((request as HttpServletRequest), (response as HttpServletResponse), ex)
        }
    }

    companion object {
        const val ACCESS_LOG_START_TIME_ATTRIBUTE =
            "ru.yandex.alice.paskill.dialogovo.midleware.AccessLogStartTimeAttribute"
    }
}
