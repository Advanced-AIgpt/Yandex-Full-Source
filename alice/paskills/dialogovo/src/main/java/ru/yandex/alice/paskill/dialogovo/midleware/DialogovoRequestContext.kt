package ru.yandex.alice.paskill.dialogovo.midleware

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.paskill.dialogovo.scenarios.DialogovoState

@Component
class DialogovoRequestContext {
    private val holder: ThreadLocal<Context> = ThreadLocal.withInitial { Context() }
    var context: Context
        get() = holder.get()
        set(context) {
            holder.set(context)
        }
    var megaMindRequestContext: MegaMindRequestContext?
        get() = holder.get().megaMindRequestContext
        set(megaMindRequestContext) {
            holder.get().megaMindRequestContext = megaMindRequestContext
        }

    val floydRequestContext: FloydRequestContext?
        get() = holder.get().floydRequestContext

    // set only by floyd controller
    fun setFloydRequestContext(megaMindRequestContext: FloydRequestContext?) {
        holder.get().floydRequestContext = megaMindRequestContext
    }

    var webhookRequestDurationMs: Long
        get() = holder.get().webhookRequestDurationMs
        set(duration) {
            holder.get().webhookRequestDurationMs = duration
        }

    fun clear() {
        // holder.get().clear()
        holder.set(Context())
    }

    // @Nullable
    var scenario: ScenarioMeta?
        get() = holder.get().scenario
        set(scenario) {
            holder.get().scenario = scenario
        }

    data class MegaMindRequestContext(
        val dialogId: String?,
        val appId: String,
        val uuid: String,
        val deviceId: String?,
        val platform: String?,
        val voiceSession: Boolean,
        val currentSkillId: String?,
        val sessionId: String?
    ) {

        companion object {
            fun from(request: MegaMindRequest<*>): MegaMindRequestContext = MegaMindRequestContext(
                dialogId = request.dialogId,
                appId = request.clientInfo.appId,
                uuid = request.clientInfo.uuid,
                deviceId = request.clientInfo.deviceId,
                platform = request.clientInfo.platform,
                voiceSession = request.voiceSession,
                // only for scenario with DialogovoState state
                currentSkillId = (request.state as? DialogovoState)?.currentSkillId,
                sessionId = (request.state as? DialogovoState)?.session?.sessionId,
            )
        }
    }

    data class Context(
        var megaMindRequestContext: MegaMindRequestContext? = null,
        var webhookRequestDurationMs: Long = 0,
        var scenario: ScenarioMeta? = null,
        var floydRequestContext: FloydRequestContext? = null,
    ) {

        fun clear() {
            megaMindRequestContext = null
            webhookRequestDurationMs = 0L
            scenario = null
            floydRequestContext = null
        }

        fun makeCopy(): Context = Context(
            megaMindRequestContext = megaMindRequestContext?.copy(),
            webhookRequestDurationMs = this.webhookRequestDurationMs,
            scenario = this.scenario,
            floydRequestContext = this.floydRequestContext,
        )
    }

    data class FloydRequestContext(val puid: String?, val login: String?, val operatorChatId: String?)
}
