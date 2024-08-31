package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TThereminPlayActionPayload
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TThereminPlayActionPayload.TThereminInstrument
import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TThereminPlayActionPayload.TThereminStartPhraseType
import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TAction

class ThereminPlayAnalyticsInfo(instrument: GenericInstument, startPhraseType: StartPhraseType) :
    AnalyticsInfo(intent = THEREMIN_PLAY, actions = listOf(Action(instrument, startPhraseType))) {

    enum class StartPhraseType(internal val protoEnum: TThereminStartPhraseType) {
        Full(TThereminStartPhraseType.Full),
        Short(TThereminStartPhraseType.Short),
    }

    class Action(
        val instrument: GenericInstument,
        val startPhraseType: StartPhraseType,
    ) : AnalyticsInfoAction(
        "theremin.play",
        "theremin.play",
        "Включение режима синтезатора, звук ${instrument.humanReadableName}"
    ) {

        override fun fillProtoField(protoBuilder: TAction.Builder): TAction.Builder {
            return protoBuilder.setThereminPlay(
                TThereminPlayActionPayload.newBuilder()
                    .setInstrument(
                        TThereminInstrument.newBuilder()
                            .setId(instrument.id)
                            .setDeveloperType(DeveloperType.getProtoDeveloperType(instrument.developerType))
                            .setIsPublic(instrument.public)
                    )
                    .setStartPhraseType(startPhraseType.protoEnum)
            )
        }
    }
}
