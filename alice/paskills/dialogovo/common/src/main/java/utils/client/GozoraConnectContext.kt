package ru.yandex.alice.paskill.dialogovo.utils.client

import org.springframework.stereotype.Component

data class GozoraConnectContext(
    val requestId: String?,
    val disableSslValidation: Boolean,
) {
    companion object {
        @JvmStatic
        val EMPTY = GozoraConnectContext(null, false)
    }
}

@Component
class GozoraConnectContextHolder {

    private val holder: ThreadLocal<GozoraConnectContext> = ThreadLocal.withInitial { GozoraConnectContext.EMPTY }

    fun setContext(context: GozoraConnectContext) {
        holder.set(context)
    }

    fun setContext(requestId: String?, disableSslValidation: Boolean) {
        setContext(GozoraConnectContext(requestId, disableSslValidation))
    }

    fun getContext(): GozoraConnectContext {
        return holder.get()
    }

    fun clear() {
        holder.set(GozoraConnectContext.EMPTY)
    }
}
