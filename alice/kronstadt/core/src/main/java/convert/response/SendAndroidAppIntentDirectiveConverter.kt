package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.SendAndroidAppIntentDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.megamind.protos.scenarios.directive.TSendAndroidAppIntentDirective

@Component
open class SendAndroidAppIntentDirectiveConverter : DirectiveConverterBase<SendAndroidAppIntentDirective> {
    override val directiveType: Class<SendAndroidAppIntentDirective>
        get() = SendAndroidAppIntentDirective::class.java

    override fun convert(src: SendAndroidAppIntentDirective, ctx: ToProtoContext): TDirective {
        val protoDirectiveBuilder = TSendAndroidAppIntentDirective.newBuilder()
            .setName("send_android_app_intent")
            .setAction(src.action)
            .setStartType(convertStartType(src.startType))

        src.uri?.apply { protoDirectiveBuilder.uri = src.uri }
        src.category?.apply { protoDirectiveBuilder.category = src.category }
        src.type?.apply { protoDirectiveBuilder.type = src.type }
        src.component?.apply { protoDirectiveBuilder.component = convertComponent(src.component) }

        if (src.flags.isNotEmpty()) {
            val flags = protoDirectiveBuilder.flagsBuilder
            src.flags.forEach {
                when(it) {
                    SendAndroidAppIntentDirective.IntentFlag.FLAG_ACTIVITY_NEW_TASK -> flags.flagActivityNewTask = true
                }
            }
        }

        return TDirective.newBuilder()
            .setSendAndroidAppIntentDirective(protoDirectiveBuilder)
            .build()
    }

    private fun convertComponent(component: SendAndroidAppIntentDirective.Component): TSendAndroidAppIntentDirective.TComponent {
        return TSendAndroidAppIntentDirective.TComponent.newBuilder()
            .setCls(component.cls)
            .setPkg(component.pkg)
            .build()
    }

    private fun convertStartType(startType: SendAndroidAppIntentDirective.StartType): TSendAndroidAppIntentDirective.EIntentStartType {
        return when (startType) {
            SendAndroidAppIntentDirective.StartType.START_ACTIVITY -> TSendAndroidAppIntentDirective.EIntentStartType.StartActivity
            SendAndroidAppIntentDirective.StartType.SEND_BROADCAST -> TSendAndroidAppIntentDirective.EIntentStartType.SendBroadcast
        }
    }
}
