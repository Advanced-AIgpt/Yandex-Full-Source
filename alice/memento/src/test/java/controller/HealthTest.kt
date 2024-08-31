package ru.yandex.alice.memento.controller

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.autoconfigure.http.HttpMessageConverters
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.web.client.RestTemplateBuilder
import org.springframework.boot.web.server.LocalServerPort
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.test.context.ActiveProfiles
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.exchange
import ru.yandex.alice.memento.storage.TestStorageConfiguration
import ru.yandex.alice.memento.storage.ydb.TestYdbConfiguration
import ru.yandex.alice.memento.tvm.TestTvmConfiguration
import java.nio.charset.StandardCharsets

@SpringBootTest(
    classes = [TestTvmConfiguration::class, TestStorageConfiguration::class, TestYdbConfiguration::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
    properties = ["ydb.warmup.waveCount=1", "ydb.warmup.queriesInWave=1"]
)
@ActiveProfiles("ut")
class HealthTest {

    @Autowired
    private lateinit var messageConverter: HttpMessageConverters

    @LocalServerPort
    private val port = 0

    private lateinit var restTemplate: RestTemplate

    @BeforeEach
    fun setUp() {
        restTemplate = RestTemplateBuilder().messageConverters(messageConverter.converters).build()
    }

    @Test
    internal fun testHealthActuator() {
        val headers = HttpHeaders()
        headers.accept = listOf()
        assertEquals(
            "{\"status\":\"UP\"}",
            restTemplate.exchange<ByteArray>(
                "http://localhost:${port}/actuator/health/readiness",
                method = HttpMethod.GET,
                requestEntity = HttpEntity(null, headers)
            ).body?.let { String(it, StandardCharsets.UTF_8) }
        )
    }
}
