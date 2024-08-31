package ru.yandex.alice.kronstadt.scenarios.alice4business

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.beans.factory.annotation.Value
import org.springframework.dao.DuplicateKeyException
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.http.ResponseEntity
import org.springframework.scheduling.annotation.Scheduled
import org.springframework.stereotype.Component
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.tvm.SERVICE_TICKET_HEADER
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.ActivationCode
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.DeviceStatus
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.dto.DeviceIdentifier
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.dto.GetDevicesListResponse
import ru.yandex.alice.kronstadt.scenarios.alice4business.providers.ActivationCodeDao
import ru.yandex.alice.kronstadt.scenarios.alice4business.providers.BusinessDeviceDao
import ru.yandex.alice.paskills.common.solomon.utils.Instrument
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.passport.tvmauth.TvmClient
import java.sql.Timestamp
import java.time.Instant
import java.util.Random
import java.util.UUID
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
import kotlin.streams.asSequence

private const val ACTIVATION_CODE_SIZE = 4L

@Component
internal class Alice4BusinessServiceImpl(
    @Qualifier("alice4BusinessRestTemplate") private val webClient: RestTemplate,
    private val tvmClient: TvmClient,
    private val businessDeviceDao: BusinessDeviceDao,
    private val activationCodeDao: ActivationCodeDao,
    @Value("\${alice4-business-config.url}")
    private val url: String,
    @Value("\${customer-activation-operation.codeRefreshInterval}")
    private val codeRefreshInterval: Long,
    @Value("\${customer-activation-operation.stationUrl}")
    private val stationUrl: String,
    @Value("\${alice4-business-config.getDeviceLockStateTimeout}")
    private val getDeviceLockStateTimeout: Long,
    @Value("\${alice4-business-config.activationCodeCollisionRepeats}")
    private val activationCodeCollisionRepeats: Int,
    @Value("\${alice4-business-config.fillCachesOnStartUp}")
    fillCachesOnStartUp: Boolean,
    internalMetricRegistry: MetricRegistry
) : Alice4BusinessService {

    private val getActivationLockStateInstrument: Instrument
    private val getOrGenerateActivationCodeInstrument: Instrument
    private val activationCodeGenerationCollisionsRate: Rate

    init {
        val registry = NamedSensorsRegistry(internalMetricRegistry, "alice4businessService")

        getActivationLockStateInstrument = registry.instrument("getActivationLockState")
        getOrGenerateActivationCodeInstrument = registry.instrument("getOrGenerateActivationCode")
        activationCodeGenerationCollisionsRate = registry.rate("activationCodeGenerationCollisions")
    }

    @Volatile
    internal var devices: Map<String, DeviceIdentifier> = mapOf()

    @Volatile
    override var isReady: Boolean = false
        private set

    private val logger = LogManager.getLogger()

    init {
        if (fillCachesOnStartUp) {
            loadDevices()
        }
    }

    @Scheduled(initialDelay = 60000, fixedRate = 60000)
    private fun loadDevices() {
        try {
            devices = getBusinessDevices()
            if (!isReady) {
                isReady = true
            }
        } catch (ignored: Throwable) {
            logger.error("Exception while loading devices", ignored)
        }
    }

    fun getBusinessDevices(): Map<String, DeviceIdentifier> {
        try {
            logger.trace("Load business device list")
            val url = UriComponentsBuilder.fromUriString(url)
                .pathSegment("devices")
                .build()
                .toUri()

            val headers = HttpHeaders()
            val serviceTicket = tvmClient.getServiceTicketFor("alice4business")

            headers.add(SERVICE_TICKET_HEADER, serviceTicket)
            val response: ResponseEntity<GetDevicesListResponse> =
                webClient.exchange(url, HttpMethod.GET, HttpEntity<Any>(headers))

            logger.trace("Got raw business device list, parsing result")
            val result: Map<String, DeviceIdentifier> = response.body?.result?.associateBy { it.deviceId }
                ?: throw NoSuchElementException("devices not found")

            logger.info("Business devices list was loaded with {} entries", result.size)
            return result
        } catch (e: Throwable) {
            logger.error("Failed to load business device list", e)
            throw e
        }
    }

    override fun isBusinessDevice(deviceId: String?): Boolean {
        return deviceId != null && devices.containsKey(deviceId)
    }

    override fun getDeviceLockState(deviceId: String?, requestTime: Instant): DeviceLockState? =
        try {
            getActivationLockStateInstrument.time<DeviceLockState?> {
                CompletableFuture.supplyAsync<DeviceLockState?> {

                    logger.info("Loading device lock state for $deviceId")
                    val deviceIdentifier = devices[deviceId] ?: return@supplyAsync null

                    val device = businessDeviceDao.get(
                        deviceIdentifier.deviceId,
                        deviceIdentifier.platform
                    ) ?: return@supplyAsync null

                    when (device.status) {
                        DeviceStatus.Active, DeviceStatus.Unknown -> {
                            logger.info("Device(id=${deviceIdentifier.deviceId}) is unlocked")
                            return@supplyAsync DeviceLockState(locked = false)
                        }
                        DeviceStatus.Inactive, DeviceStatus.Reset -> {
                            logger.info("Device(id=${deviceIdentifier.deviceId}) is locked")
                            val nowMs = requestTime.toEpochMilli()
                            val activationCode = getOrGenerateActivationCodeInstrument.time<ActivationCode?> {
                                activationCodeDao.getByDeviceIdCreatedLaterThan(
                                    device.id,
                                    Timestamp(nowMs - codeRefreshInterval)
                                ) ?: createNewActivationCode(device.id, Timestamp(nowMs))
                            }
                            return@supplyAsync DeviceLockState(
                                locked = true,
                                code = activationCode.code,
                                stationUrl = stationUrl
                            )
                        }
                    }
                }.get(getDeviceLockStateTimeout, TimeUnit.MILLISECONDS)
            }
        } catch (e: TimeoutException) {
            logger.error("Timed out waiting for device lock state", e)
            null
        } catch (e: Throwable) {
            logger.error("Failed to get device lock state", e)
            null
        }

    private fun createNewActivationCode(deviceId: UUID, createdAt: Timestamp): ActivationCode {
        repeat(activationCodeCollisionRepeats) {
            val code = Random().ints(ACTIVATION_CODE_SIZE, 0, 10).asSequence().joinToString("")
            try {
                return activationCodeDao.save(
                    code = code,
                    deviceId = deviceId,
                    createdAt = createdAt
                )
            } catch (e: DuplicateKeyException) {
                activationCodeGenerationCollisionsRate.inc()
            }
        }
        throw RuntimeException("Too many collisions with existing activation codes, failed to generate new activation code")
    }
}
