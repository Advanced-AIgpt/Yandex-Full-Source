package ru.yandex.alice.kronstadt.core

import org.springframework.stereotype.Component

/**
 * Class that holds Http/gRPC request context, needed to call other services and for logging purposes.
 * There is no data from MM request. If one needs a threadlocal context with MM specific data use
 * [MegaMindRequestListener] which may be passed to
 * [AbstractScenario]
 */
@Component
class RequestContext {
    private val holder: ThreadLocal<Context> = ThreadLocal.withInitial { Context() }
    var context: Context
        get() = holder.get()
        set(context) {
            holder.set(context)
        }
    var requestId: String?
        get() = holder.get().requestId
        set(reqId) {
            holder.get().requestId = reqId
        }
    var currentUserId: String?
        get() = holder.get().currentUserId
        set(uid) {
            holder.get().currentUserId = uid
        }
    var currentUserTicket: String?
        get() = holder.get().currentUserTicket
        set(userTicket) {
            holder.get().currentUserTicket = userTicket
        }
    var forwardedFor: String?
        get() = holder.get().forwardedFor
        set(value) {
            holder.get().forwardedFor = value
        }
    var userAgent: String?
        get() = holder.get().userAgent
        set(value) {
            holder.get().userAgent = value
        }
    var sourceTvmClientId: Int?
        get() = holder.get().sourceTvmClientId
        set(value) {
            holder.get().sourceTvmClientId = value
        }
    var oauthToken: String?
        get() = holder.get().oauthToken
        set(value) {
            holder.get().oauthToken = value
        }

    fun clear() {
        holder.get().clear()
    }

    data class Context internal constructor(
        internal var requestId: String? = null,
        internal var currentUserId: String? = null,
        internal var currentUserTicket: String? = null,
        internal var sourceTvmClientId: Int? = null,
        internal var forwardedFor: String? = null,
        internal var userAgent: String? = null,
        internal var oauthToken: String? = null,
    ) {

        fun clear() {
            requestId = null
            currentUserId = null
            currentUserTicket = null
            forwardedFor = null
            userAgent = null
            sourceTvmClientId = null
            oauthToken = null
        }

        fun makeCopy(): Context {
            return Context(
                requestId,
                currentUserId,
                currentUserTicket,
                sourceTvmClientId,
                forwardedFor,
                userAgent,
                oauthToken,
            )
        }
    }
}
