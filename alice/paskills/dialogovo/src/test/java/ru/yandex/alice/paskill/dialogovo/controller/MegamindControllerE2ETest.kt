package ru.yandex.alice.paskill.dialogovo.controller

import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.readValue
import org.apache.logging.log4j.LogManager
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertNotEquals
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.Arguments
import org.junit.jupiter.params.provider.MethodSource
import org.skyscreamer.jsonassert.JSONAssert
import org.skyscreamer.jsonassert.JSONCompareMode
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.http.HttpEntity
import org.springframework.http.HttpMethod
import org.springframework.http.HttpStatus
import ru.yandex.alice.kronstadt.test.DynamicValueTokenComparator
import ru.yandex.alice.kronstadt.test.SideEffectRequestType
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto.TScenarioRunResponse
import ru.yandex.alice.paskill.dialogovo.config.E2EConfigProvider
import ru.yandex.alice.paskill.dialogovo.controller.BaseControllerE2ETest.SyncExecutorsConfiguration
import java.io.File
import java.time.Instant
import java.util.Locale
import java.util.UUID

@SpringBootTest(
    classes = [E2EConfigProvider::class, SyncExecutorsConfiguration::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
@AutoConfigureWebClient(registerRestTemplate = true)
internal class MegamindControllerE2ETest : BaseControllerE2ETest() {

    @ParameterizedTest(name = "{0}")
    @MethodSource("getApplyTestData")
    fun applyEndpointTest(name: String, path: String, baseUrl: String) {
        logger.debug("start test case")
        val dir = File(path)
        val uuidReplacement = UUID.randomUUID().toString()

        mockServices(dir, uuidReplacement, Instant.now())

        val requestJson = readContent("run_request.json")
            ?.replace("\"<TIMESTAMP>\"".toRegex(), System.currentTimeMillis().toString())
            ?.replace("<UUID>".toRegex(), UUID.randomUUID().toString())

        val headers = getHttpHeadersWithJson()
        val node = objectMapper.readValue(requestJson, ObjectNode::class.java)
        val runRequest = objectMapper.writeValueAsString(node)
        val runResponse = restTemplate.exchange(
            "http://localhost:${port}$baseUrl/run",
            HttpMethod.POST,
            HttpEntity(runRequest, headers),
            String::class.java
        )

        Assertions.assertEquals(runResponse.statusCode, HttpStatus.OK)

        val runResponseProtoBuilder = TScenarioRunResponse.newBuilder()
        protoJsonParser.merge(runResponse.body, runResponseProtoBuilder)
        val runResponseProto = runResponseProtoBuilder.build()

        validateResponse("run_response.json", runResponse)

        val sideEffectType = SideEffectRequestType.fromRunResponse(runResponseProto)

        val intent: String
        if (sideEffectType.hasSideEffects()) {
            val runResponseNode = objectMapper.readValue<ObjectNode>(runResponse.body)

            val sideEffectRequestNode = node.deepCopy()
            sideEffectRequestNode.set<JsonNode>("arguments", sideEffectType.getApplyArguments(runResponseNode))

            val realSideEffectRequest = objectMapper.writeValueAsString(sideEffectRequestNode)
            val realApplyResponse = restTemplate.exchange(
                sideEffectType.getUrl(port, baseUrl),
                HttpMethod.POST,
                HttpEntity(realSideEffectRequest, headers),
                String::class.java
            )

            val actualSideEffectResponse = realApplyResponse.body
            Assertions.assertEquals(runResponse.statusCode, HttpStatus.OK)
            val expectedSideEffectResponse = readContent(sideEffectType.mockResponseFilename)

            if (expectedSideEffectResponse != null) {
                try {
                    JSONAssert.assertEquals(
                        expectedSideEffectResponse,
                        actualSideEffectResponse,
                        DynamicValueTokenComparator(JSONCompareMode.STRICT)
                    )
                } catch (e: AssertionError) {
                    logger.warn("Assertion failure:$actualSideEffectResponse", e)
                    throw e
                }
            } else {
                throw RuntimeException(String.format("no %s file", sideEffectType.mockResponseFilename))
            }

            if (sideEffectType == SideEffectRequestType.COMMIT) {
                intent = runResponseProto.commitCandidate.responseBody.analyticsInfo.intent
            } else {
                val applyResponseProtoBuilder = ResponseProto.TScenarioApplyResponse.newBuilder()
                protoJsonParser.merge(actualSideEffectResponse, applyResponseProtoBuilder)
                val applyResponseProto = applyResponseProtoBuilder.build()
                intent = applyResponseProto.responseBody.analyticsInfo.intent
            }
        } else {
            intent = runResponseProto.responseBody.analyticsInfo.intent
        }

        assertNotEquals(intent, "", "Intent cannot be empty")
        // default intent value is set to "unknown" in AnalyticsInfoContext
        assertNotEquals(intent.lowercase(Locale.ROOT), "unknown", "Intent cannot be \"unknown\"")

        validateRequests(dir)
        logger.debug("finish test case")
    }

    companion object {
        private val logger = LogManager.getLogger()

        @JvmStatic
        fun getApplyTestData(): List<Arguments> {
            return getTestDataFromDir("integration/*")
        }
    }
}
