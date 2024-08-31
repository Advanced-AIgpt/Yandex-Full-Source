package ru.yandex.alice.kronstadt.core.convert.response

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse.TFeatures

@Component
open class FeaturesConverter {
    fun convert(src: BaseRunResponse<*>): TFeatures {
        val features: TFeatures.Builder = TFeatures.newBuilder()
            .setIsIrrelevant(!src.isRelevant)
        src.features?.playerFeatures?.let { playerFeatures ->
            features.setPlayerFeatures(
                TFeatures.TPlayerFeatures
                    .newBuilder()
                    .setRestorePlayer(playerFeatures.restorePlayer)
                    .setSecondsSincePause(playerFeatures.secondsSincePause.toInt())
            )
        }
        return features.build()
    }
}
