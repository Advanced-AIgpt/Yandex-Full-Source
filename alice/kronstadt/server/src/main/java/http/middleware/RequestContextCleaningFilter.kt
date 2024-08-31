package ru.yandex.alice.kronstadt.server.http.middleware

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.RequestContext
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse

@Order(1)
@Component
class RequestContextCleaningFilter(private val requestContext: RequestContext) : Filter {

    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        requestContext.clear()
        try {
            chain.doFilter(request, response)
        } finally {
            requestContext.clear()
        }
    }
}
