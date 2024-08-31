package ru.yandex.alice.kronstadt.core.directive

data class SendAndroidAppIntentDirective(
    val action: String,
    val uri: String? = null,
    val category: String? = null,
    val type: String? = null,
    val component: Component? = null,
    val startType: StartType = StartType.START_ACTIVITY,
    val flags: Set<IntentFlag> = setOf()
) : MegaMindDirective {

    data class Component(
        val pkg: String,
        val cls: String
    )

    enum class StartType {
        START_ACTIVITY,
        SEND_BROADCAST,
    }

    // https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto?rev=r8848676#L2211
    enum class IntentFlag {
        FLAG_ACTIVITY_NEW_TASK,
    }
}

