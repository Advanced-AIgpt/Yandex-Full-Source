package ru.yandex.alice.memento.controller

import org.slf4j.LoggerFactory
import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityRequestAttributes
import java.io.IOException
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletException
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

@Order(10)
@Component
internal class AccessLogFilter : Filter {
    private val forwardersRegex = ",\\s*".toRegex()

    @Throws(IOException::class, ServletException::class)
    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        val start = System.nanoTime()
        val httpRequest = request as? HttpServletRequest
        val httpResponse = response as? HttpServletResponse
        try {
            chain.doFilter(request, response)
        } finally {
            val realIpHeader = httpRequest?.getHeader("X-Real-IP").trimToNull()
            val forwarders = httpRequest?.getHeader(X_FORWARDED_FOR).trimToNull()
            val forwardedFor = forwarders?.let { forwardersRegex.split(it, limit = 1) }?.firstOrNull()
            val tvmClientId = httpRequest?.getAttribute(SecurityRequestAttributes.TVM_CLIENT_ID_REQUEST_ATTR) as String?
            val uid = httpRequest?.getAttribute(SecurityRequestAttributes.UID_REQUEST_ATTR) as Long?
            val requestDuration = (System.nanoTime() - start) / 1000000

            // logger.trace("{} {} {} \"{} {} {}\" {} \"{}\" \"{}\" {} {} {} {} {} {} {} \"{}\" {} {} {} \"{}\" {}",
            logger.trace(
                "{} {} {} {} \"{} {} {}\" {} {} {} {} \"{}\" {}",
                request.serverName ?: UNDEFINED, // $http_host
                realIpHeader ?: request.remoteAddr ?: UNDEFINED, // $remote_addr
                forwardedFor ?: UNDEFINED, // $x_forwarded_for1
                httpRequest?.getHeader("X-Request-ID") ?: UNDEFINED, // $request_id
                httpRequest?.method ?: UNDEFINED, // "$request_method"
                httpRequest?.requestURI ?: UNDEFINED, // "$request_path"
                request.protocol ?: UNDEFINED, // "$request_protocol"
                httpResponse?.status, // $status
                tvmClientId ?: UNDEFINED, // $tvm_client_id
                uid ?: UNDEFINED, // $uid from tvm user ticket
                requestDuration, // $request_time
                forwarders ?: UNDEFINED, // "$forwarders"
                "${request.remoteAddr}:${request.remotePort}" ?: UNDEFINED // $raw_remote_addr
            )
        }
    }

    private fun String?.trimToNull(): String? = this?.trim()?.takeIf { it.isNotEmpty() }

    companion object {
        private val logger = LoggerFactory.getLogger("ACCESS_LOG")
        private const val UNDEFINED = "-"
        private const val X_FORWARDED_FOR = "X-Forwarded-For"
    }
}
