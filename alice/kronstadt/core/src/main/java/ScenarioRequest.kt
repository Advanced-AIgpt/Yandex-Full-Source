package ru.yandex.alice.kronstadt.core

import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import java.time.Instant
import java.util.Random

interface ScenarioRequest : WithExperiments {
    val requestId: String
    val scenarioMeta: ScenarioMeta
    val serverTime: Instant
    val randomSeed: Long
    override val experiments: Set<String>
    val clientInfo: ClientInfo
    val random: Random
    val mementoData: RequestProto.TMementoData
    val additionalSources: AdditionalSources
    val krandom: kotlin.random.Random
}
