package ru.yandex.alice.kronstadt.core.rpc

import com.google.protobuf.Message
import ru.yandex.alice.kronstadt.core.AdditionalSources
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.ScenarioRequest
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.megamind.protos.scenarios.RequestProto
import java.time.Instant
import java.util.Random
import kotlin.random.asKotlinRandom

data class RpcRequest<T : Message>(
    override val requestId: String,
    val requestBody: T,
    override val scenarioMeta: ScenarioMeta,
    override val clientInfo: ClientInfo,
    val userId: String? = null,
    override val serverTime: Instant = Instant.now(),
    override val randomSeed: Long = kotlin.random.Random.nextLong(),
    override val experiments: Set<String> = setOf(),
    override val random: Random = Random(randomSeed),
    override val additionalSources: AdditionalSources = AdditionalSources.EMPTY,
    override val mementoData: RequestProto.TMementoData = RequestProto.TMementoData.getDefaultInstance(),
    //val locationInfo: LocationInfo? = null,
    override val krandom: kotlin.random.Random = random.asKotlinRandom()
) : ScenarioRequest

