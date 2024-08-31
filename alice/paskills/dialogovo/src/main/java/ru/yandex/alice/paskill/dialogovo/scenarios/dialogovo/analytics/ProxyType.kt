package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics

import ru.yandex.alice.kronstadt.core.utils.StringEnum
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo

enum class ProxyType(
    val protoValue: Dialogovo.TRequestSkillWebhookEvent.EProxy,
    private val value: String,
) : StringEnum {
    DIRECT(Dialogovo.TRequestSkillWebhookEvent.EProxy.DIRECT, "direct"),
    GOZORA(Dialogovo.TRequestSkillWebhookEvent.EProxy.GOZORA, "gozora");

    override fun value(): String {
        return value
    }
}
