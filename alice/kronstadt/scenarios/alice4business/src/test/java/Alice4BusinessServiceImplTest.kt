package ru.yandex.alice.kronstadt.scenarios.alice4business

import com.google.common.collect.Maps
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import okhttp3.mockwebserver.RecordedRequest
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.mockito.ArgumentMatchers
import org.mockito.Mockito
import org.mockito.Mockito.`when`
import org.mockito.Mockito.clearInvocations
import org.mockito.Mockito.verify
import org.springframework.boot.autoconfigure.EnableAutoConfiguration
import org.springframework.boot.test.autoconfigure.web.client.RestClientTest
import org.springframework.boot.test.mock.mockito.MockBean
import org.springframework.context.annotation.Configuration
import org.springframework.web.client.RestTemplate
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.ActivationCode
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.BusinessDeviceWithStatus
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.DeviceStatus
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.dto.DeviceIdentifier
import ru.yandex.alice.kronstadt.scenarios.alice4business.providers.ActivationCodeDao
import ru.yandex.alice.kronstadt.scenarios.alice4business.providers.BusinessDeviceDao
import ru.yandex.monlib.metrics.registry.MetricRegistry
import ru.yandex.passport.tvmauth.TvmClient
import java.io.IOException
import java.sql.Timestamp
import java.time.Instant
import java.util.UUID
import java.util.concurrent.TimeUnit
import javax.sql.DataSource
import ru.yandex.alice.kronstadt.test.mockito.any as kotlinAny
import ru.yandex.alice.kronstadt.test.mockito.eq as kotlinEq

private const val DEVICE_ID = "04107884c914600809cf"
private const val PLATFORM = "yandexstation"
private const val CODE = "123"
private const val STATION_URL = "https://yandex.ru"

@RestClientTest(components = [Alice4BusinessServiceImplTest.TestConfiguration::class])
@MockBean(DataSource::class)
internal class Alice4BusinessServiceImplTest {
    private lateinit var mockAlice4BusinessServer: MockWebServer
    private lateinit var alice4BusinessService: Alice4BusinessServiceImpl
    private lateinit var now: Instant

    @MockBean
    private lateinit var tvmClient: TvmClient

    @MockBean
    private lateinit var activationCodeDao: ActivationCodeDao

    @MockBean
    private lateinit var businessDeviceDao: BusinessDeviceDao

    @BeforeEach
    @Throws(IOException::class)
    fun setUp() {
        mockAlice4BusinessServer = MockWebServer()
        mockAlice4BusinessServer.start()
        `when`(tvmClient.getServiceTicketFor(ArgumentMatchers.eq("alice4business")))
            .thenReturn("xxx-ticket")

        alice4BusinessService = Alice4BusinessServiceImpl(
            webClient = RestTemplate(),
            tvmClient = tvmClient,
            businessDeviceDao = businessDeviceDao,
            activationCodeDao = activationCodeDao,
            url = mockAlice4BusinessServer.url("/").url().toString(),
            codeRefreshInterval = 1,
            stationUrl = STATION_URL,
            fillCachesOnStartUp = false,
            getDeviceLockStateTimeout = 200,
            internalMetricRegistry = MetricRegistry(),
            activationCodeCollisionRepeats = 5
        )

        now = Instant.now()
    }

    @AfterEach
    @Throws(IOException::class)
    fun tearDown() {
        mockAlice4BusinessServer.shutdown()
        Mockito.reset(tvmClient)
    }

    private fun validateNoRequests() {
        val req = Assertions.assertDoesNotThrow<RecordedRequest> {
            mockAlice4BusinessServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        }
        Assertions.assertNull(req)
    }

    private fun validateGetBusinessDevicesRequest() {
        val req = Assertions.assertDoesNotThrow<RecordedRequest> {
            mockAlice4BusinessServer.takeRequest(1L, TimeUnit.MILLISECONDS)
        }
        Assertions.assertNotNull(req)
        Assertions.assertEquals("GET", req.method)
        Assertions.assertEquals("/devices", req.path)
        Assertions.assertEquals("xxx-ticket", req.getHeader("X-Ya-Service-Ticket"))
    }

    @Test
    fun businessDevices_BadResult1() {
        mockAlice4BusinessServer.enqueue(
            MockResponse()
        )
        Assertions.assertThrows(NoSuchElementException::class.java) { alice4BusinessService.getBusinessDevices() }
        validateGetBusinessDevicesRequest()
    }

    @Test
    fun businessDevices_BadResult2() {
        mockAlice4BusinessServer.enqueue(
            MockResponse()
                .addHeader("Content-Type: application/json;charset=UTF-8")
                .setBody("{}")
        )
        Assertions.assertThrows(NoSuchElementException::class.java) { alice4BusinessService.getBusinessDevices() }
        validateGetBusinessDevicesRequest()
    }

    @Test
    fun businessDevices_EmptyResult() {
        mockAlice4BusinessServer.enqueue(
            MockResponse()
                .addHeader("Content-Type: application/json;charset=UTF-8")
                .setBody("{\"result\":[]}")
        )
        Assertions.assertTrue(alice4BusinessService.getBusinessDevices().isEmpty())
        validateGetBusinessDevicesRequest()
    }

    @Test
    fun businessDevices() {
        mockAlice4BusinessServer.enqueue(
            MockResponse()
                .addHeader("Content-Type: application/json;charset=UTF-8")
                .setBody(
                    """{"result":[{"platform":"$PLATFORM","deviceId":"$DEVICE_ID"},{"platform":"yandexmini","deviceId":"FF98F0293D3B2D3A8D406FAB"}]}"""
                )
        )
        Assertions.assertTrue(
            Maps.difference(
                mapOf(
                    DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID),
                    "FF98F0293D3B2D3A8D406FAB" to DeviceIdentifier("yandexmini", "FF98F0293D3B2D3A8D406FAB")
                ),
                alice4BusinessService.getBusinessDevices()
            ).areEqual()
        )
        validateGetBusinessDevicesRequest()
    }

    @Test
    fun isBusinessDevice() {
        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        Assertions.assertTrue(alice4BusinessService.isBusinessDevice(DEVICE_ID))
        Assertions.assertFalse(
            alice4BusinessService.isBusinessDevice("04107884C914600809CF"),
            "Register mismatch"
        )
        Assertions.assertFalse(alice4BusinessService.isBusinessDevice("FF98F0293D3B2D3A8D406FAB"))
        Assertions.assertFalse(alice4BusinessService.isBusinessDevice(""))
        Assertions.assertFalse(alice4BusinessService.isBusinessDevice(null))
        validateNoRequests()
    }

    @Test
    fun deviceLockState_UnknownDevice() {
        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        // Device in not in database
        `when`(
            businessDeviceDao.get(
                kotlinAny(String::class.java),
                kotlinAny(String::class.java)
            )
        ).thenReturn(null)

        Assertions.assertNull(alice4BusinessService.getDeviceLockState(DEVICE_ID, now))
        Assertions.assertNull(alice4BusinessService.getDeviceLockState("FF98F0293D3B2D3A8D406FAB", now))
        Assertions.assertNull(alice4BusinessService.getDeviceLockState("", now))
        Assertions.assertNull(alice4BusinessService.getDeviceLockState(null, now))
        validateNoRequests()
    }

    @Test
    fun deviceLockState_DeviceNotInBusinessDevice() {
        val uuid = UUID.randomUUID()
        val deviceStatus = DeviceStatus.Active

        alice4BusinessService.devices = mapOf()

        `when`(businessDeviceDao.get(DEVICE_ID, PLATFORM)).thenReturn(
            BusinessDeviceWithStatus(
                uuid,
                PLATFORM,
                deviceStatus
            )
        )
        Assertions.assertNull(alice4BusinessService.getDeviceLockState(DEVICE_ID, now))
    }

    @Test
    fun deviceLockState_Locked_ActivationCodeExists() {
        val uuid = UUID.randomUUID()
        val activationCode = ActivationCode(CODE, uuid, Timestamp(1))
        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        `when`(
            activationCodeDao.getByDeviceIdCreatedLaterThan(
                kotlinEq(uuid),
                kotlinAny(Timestamp::class.java)
            )
        ).thenReturn(activationCode)

        for (deviceStatus in listOf(DeviceStatus.Inactive, DeviceStatus.Reset)) {
            `when`(businessDeviceDao.get(DEVICE_ID, PLATFORM)).thenReturn(
                BusinessDeviceWithStatus(
                    uuid,
                    PLATFORM,
                    deviceStatus
                )
            )

            val lockState = alice4BusinessService.getDeviceLockState(DEVICE_ID, now)!!
            Assertions.assertEquals(true, lockState.locked)
            Assertions.assertEquals(activationCode.code, lockState.code)
        }
    }

    @Test
    fun deviceLockState_Locked_ActivationCodeDoesntExist() {
        val uuid = UUID.randomUUID()
        val activationCode = ActivationCode(CODE, uuid, Timestamp(1))
        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        `when`(
            activationCodeDao.getByDeviceIdCreatedLaterThan(
                kotlinEq(uuid),
                kotlinAny(Timestamp::class.java)
            )
        ).thenReturn(null)

        `when`(
            activationCodeDao.save(
                kotlinAny(String::class.java),
                kotlinEq(uuid),
                kotlinAny(Timestamp::class.java)
            )
        ).thenReturn(activationCode)

        for (deviceStatus in listOf(DeviceStatus.Inactive, DeviceStatus.Reset)) {
            `when`(businessDeviceDao.get(DEVICE_ID, PLATFORM)).thenReturn(
                BusinessDeviceWithStatus(
                    uuid,
                    PLATFORM,
                    deviceStatus
                )
            )

            val lockState = alice4BusinessService.getDeviceLockState(DEVICE_ID, now)!!
            Assertions.assertEquals(true, lockState.locked)
            Assertions.assertEquals(activationCode.code, lockState.code)

            verify(activationCodeDao).save(
                kotlinAny(String::class.java),
                kotlinEq(uuid),
                kotlinAny(Timestamp::class.java)
            )
            clearInvocations(activationCodeDao)
        }
    }

    @Test
    fun deviceLockState_Unlocked() {
        val uuid = UUID.randomUUID()
        val deviceStatus = DeviceStatus.Active

        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        `when`(businessDeviceDao.get(DEVICE_ID, PLATFORM)).thenReturn(
            BusinessDeviceWithStatus(
                uuid,
                PLATFORM,
                deviceStatus
            )
        )

        Assertions.assertEquals(
            false,
            alice4BusinessService.getDeviceLockState(DEVICE_ID, now)!!.locked
        )
    }

    @Test
    fun deviceLockState_Locked_ReturnsValidStationUrl() {
        val uuid = UUID.randomUUID()
        val deviceStatus = DeviceStatus.Inactive
        val activationCode = ActivationCode(CODE, uuid, Timestamp(1))
        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )

        alice4BusinessService.devices = mapOf(
            DEVICE_ID to DeviceIdentifier(PLATFORM, DEVICE_ID)
        )
        `when`(
            activationCodeDao.getByDeviceIdCreatedLaterThan(
                kotlinEq(uuid),
                kotlinAny(Timestamp::class.java)
            )
        ).thenReturn(activationCode)
        `when`(businessDeviceDao.get(DEVICE_ID, PLATFORM)).thenReturn(
            BusinessDeviceWithStatus(
                uuid,
                PLATFORM,
                deviceStatus
            )
        )

        Assertions.assertEquals(
            STATION_URL,
            alice4BusinessService.getDeviceLockState(DEVICE_ID, now)!!.stationUrl
        )
    }

    @Configuration
    @EnableAutoConfiguration
    internal open class TestConfiguration
}
