package ru.yandex.alice.kronstadt.core.convert.response

import com.google.protobuf.Any
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.directive.server.DeletePusheDirective
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective
import ru.yandex.alice.kronstadt.core.directive.server.SendPushMessageDirective.AppType
import ru.yandex.alice.kronstadt.core.directive.server.ServerDirective
import ru.yandex.alice.megamind.protos.common.AppType.EAppType
import ru.yandex.alice.megamind.protos.scenarios.PushProto
import ru.yandex.alice.megamind.protos.scenarios.directive.TServerDirective
import ru.yandex.alice.memento.proto.MementoApiProto.TConfigKeyAnyPair

@Component
open class ServerDirectiveConverter : ToProtoConverter<ServerDirective, TServerDirective> {
    override fun convert(src: ServerDirective, ctx: ToProtoContext): TServerDirective {
        return when (src) {
            is SendPushMessageDirective -> convertSendPushMessage(src)
            is DeletePusheDirective -> convertDeletePushMessage(src)
            is MementoChangeUserObjectsDirective -> convertMementoChangeUserObjectsDirective(src)
        }
    }

    private fun convertSendPushMessage(src: SendPushMessageDirective): TServerDirective {
        val card = if (src.cardButtonText != null || src.cardTitle != null)
            PushProto.TSendPushDirective.TPersonalCard.newBuilder()
                .setSettings(
                    PushProto.TSendPushDirective.TCommon.newBuilder().apply {
                        src.cardButtonText?.let { title = it }
                        src.cardTitle?.let { text = it }
                    }
                )
                .build()
        else null

        return TServerDirective.newBuilder()
            .setSendPushDirective(
                PushProto.TSendPushDirective.newBuilder()
                    .setSettings(
                        PushProto.TSendPushDirective.TCommon.newBuilder()
                            .setTitle(src.title)
                            .setText(src.body)
                            .setLink(src.link.toASCIIString())
                            .setTtlSeconds(src.ttl.toSeconds().toInt())
                            .build()
                    )
                    .setPushId(src.pushId)
                    .setPushTag(src.pushTag)
                    .setPushMessage(
                        PushProto.TSendPushDirective.TPush.newBuilder()
                            .setThrottlePolicy(src.throttlePolicy)
                            .addAllAppTypes(src.appTypes.map { appType -> convertAppType(appType) })
                    )
                    .setRemoveExistingCards(true)
                    .apply {
                        card?.let { personalCard = card }
                    }
            ).build()
    }

    private fun convertDeletePushMessage(src: DeletePusheDirective): TServerDirective {
        return TServerDirective.newBuilder()
            .setDeletePushesDirective(
                PushProto.TDeletePushesDirective.newBuilder()
                    .setTag(src.pushTag)
                    .build()
            ).build()
    }

    private fun convertMementoChangeUserObjectsDirective(src: MementoChangeUserObjectsDirective): TServerDirective {
        val resultBuilder = TServerDirective.newBuilder()
        val mementoUserObjectsBuilder =
            resultBuilder.mementoChangeUserObjectsDirectiveBuilder.userObjectsBuilder
        src.userConfigs?.apply {
            val configs: List<TConfigKeyAnyPair> = src.userConfigs.map {
                TConfigKeyAnyPair.newBuilder().apply {
                    key = it.key
                    value = Any.pack(it.value.message)
                }.build()
            }
            mementoUserObjectsBuilder.addAllUserConfigs(configs)
        }
        src.scenarioData?.apply {
            mementoUserObjectsBuilder.scenarioData = src.scenarioData
        }
        src.surfaceScenarioData?.apply {
            val surfaceScenarioDataBuilder = mementoUserObjectsBuilder.surfaceScenarioDataBuilder
            src.surfaceScenarioData.scenarioData.forEach {
                surfaceScenarioDataBuilder.putScenarioData(it.key, it.value)
            }
        }
        return resultBuilder.build()
    }

    private fun convertAppType(appType: AppType): EAppType {
        return when (appType) {
            AppType.UNDEFINED -> EAppType.AT_UNDEFINED
            AppType.SEARCH_APP -> EAppType.AT_SEARCH_APP
            AppType.SMART_SPEAKER -> EAppType.AT_SMART_SPEAKER
            AppType.MOBILE_BROWSER -> EAppType.AT_MOBILE_BROWSER
            AppType.DESKTOP_BROWSER -> EAppType.AT_DESKTOP_BROWSER
        }
    }
}
