package ru.yandex.alice.paskill.dialogovo.midleware

import org.springframework.core.annotation.Order
import org.springframework.stereotype.Component
import javax.servlet.Filter
import javax.servlet.FilterChain
import javax.servlet.ServletRequest
import javax.servlet.ServletResponse

@Order(2)
@Component
class DialogovoRequestContextCleaningFilter(private val dialogovoRequestContext: DialogovoRequestContext) : Filter {

    override fun doFilter(request: ServletRequest, response: ServletResponse, chain: FilterChain) {
        dialogovoRequestContext.clear()
        try {
            chain.doFilter(request, response)
        } finally {
            dialogovoRequestContext.clear()
        }
    }
}
