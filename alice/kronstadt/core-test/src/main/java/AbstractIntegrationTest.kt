package ru.yandex.alice.kronstadt.test

import com.fasterxml.jackson.databind.JavaType
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.annotation.JsonDeserialize
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.readValue
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.TestInstance
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.Arguments
import org.junit.jupiter.params.provider.MethodSource
import org.skyscreamer.jsonassert.JSONAssert
import org.skyscreamer.jsonassert.JSONCompareMode
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.autoconfigure.EnableAutoConfiguration
import org.springframework.boot.autoconfigure.data.jdbc.JdbcRepositoriesAutoConfiguration
import org.springframework.boot.autoconfigure.jdbc.DataSourceAutoConfiguration
import org.springframework.boot.test.autoconfigure.web.client.AutoConfigureWebClient
import org.springframework.boot.test.context.TestConfiguration
import org.springframework.boot.web.client.RestTemplateCustomizer
import org.springframework.boot.web.server.LocalServerPort
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Primary
import org.springframework.core.env.Environment
import org.springframework.core.io.FileSystemResource
import org.springframework.core.io.Resource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.http.HttpRequest
import org.springframework.http.HttpStatus
import org.springframework.http.MediaType
import org.springframework.http.ResponseEntity
import org.springframework.http.client.ClientHttpRequestInterceptor
import org.springframework.http.client.support.HttpRequestWrapper
import org.springframework.test.context.ContextConfiguration
import org.springframework.test.context.TestPropertySource
import org.springframework.util.LinkedCaseInsensitiveMap
import org.springframework.util.MimeTypeUtils
import org.springframework.web.bind.annotation.PathVariable
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestMapping
import org.springframework.web.bind.annotation.RestController
import org.springframework.web.client.RestTemplate
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.kronstadt.core.RequestContext
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.VersionProvider
import ru.yandex.alice.kronstadt.core.utils.AnythingToStringJacksonDeserializer
import ru.yandex.alice.kronstadt.server.http.X_DEVELOPER_TRUSTED_TOKEN
import ru.yandex.alice.kronstadt.server.http.X_REQUEST_ID
import ru.yandex.alice.kronstadt.server.http.X_TRUSTED_SERVICE_TVM_CLIENT_ID
import ru.yandex.alice.kronstadt.test.AbstractIntegrationTest.BaseIntegrationTestConfiguration
import ru.yandex.alice.megamind.protos.scenarios.ResponseProto
import ru.yandex.alice.paskills.common.tvm.spring.handler.SecurityHeaders
import ru.yandex.passport.tvmauth.BlackboxEnv
import ru.yandex.passport.tvmauth.CheckedServiceTicket
import ru.yandex.passport.tvmauth.CheckedUserTicket
import ru.yandex.passport.tvmauth.ClientStatus
import ru.yandex.passport.tvmauth.TicketStatus
import ru.yandex.passport.tvmauth.TvmClient
import ru.yandex.passport.tvmauth.Unittest
import ru.yandex.passport.tvmauth.roles.Roles
import java.io.File
import java.net.URI
import java.nio.charset.StandardCharsets
import java.nio.file.Path
import java.time.Instant
import java.util.Arrays
import java.util.LinkedList
import java.util.Optional
import java.util.Queue
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.stream.Collectors
import javax.annotation.Nonnull
import javax.servlet.http.HttpServletRequest
import kotlin.io.path.div

// @SpringBootTest(
//     classes = [BaseIntegrationTestConfiguration::class],
//     webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
// )
//@ContextConfiguration(classes = [BaseIntegrationTestConfiguration::class])
@ContextConfiguration(
    classes = [
        //KronstadtServer::class,
        BaseIntegrationTestConfiguration::class,
        AbstractIntegrationTest.MockServer::class
    ]
)
@TestPropertySource(properties = ["tvm.validateServiceTicket=false"])
@TestInstance(TestInstance.Lifecycle.PER_CLASS)
@AutoConfigureWebClient(registerRestTemplate = true)
abstract class AbstractIntegrationTest(
    protected val scenarioMeta: ScenarioMeta,
    protected val baseDir: String = "integration/${scenarioMeta.name}",
) {
    protected val files: MutableMap<String, String> = HashMap()

    @Autowired
    protected lateinit var restTemplate: RestTemplate

    @LocalServerPort
    protected var port = 0

    @Autowired
    protected lateinit var requestContext: RequestContext

    @Autowired
    protected lateinit var tvmClient: MockTvmClient

    @Autowired
    protected lateinit var protoJsonParser: JsonFormat.Parser

    @Autowired
    protected lateinit var objectMapper: ObjectMapper

    @Autowired
    private lateinit var mockServer: MockServer

    @ParameterizedTest(name = "{0}")
    @MethodSource("testData")
    protected fun test(name: String, path: String, baseUrl: String) {
        logger.debug("start test case $name")
        val dir = File(path)
        val uuidReplacement = UUID.randomUUID().toString()

        mockServices(dir, uuidReplacement, Instant.now())

        val requestJson = readContent("run_request.json")
            ?.replace("\"<TIMESTAMP>\"".toRegex(), System.currentTimeMillis().toString())
            ?.replace("<UUID>".toRegex(), UUID.randomUUID().toString())

        val headers = getHttpHeadersWithJson()
        val node = objectMapper.readValue(requestJson, ObjectNode::class.java)
        val runRequest = objectMapper.writeValueAsString(node)
        val runResponse: ResponseEntity<String> = restTemplate.exchange(
            "http://localhost:${port}${baseUrl}/run",
            HttpMethod.POST,
            HttpEntity(runRequest, headers),
            String::class.java
        )

        assertEquals(runResponse.statusCode, HttpStatus.OK)

        val runResponseProto = ResponseProto.TScenarioRunResponse.newBuilder().apply {
            protoJsonParser.merge(runResponse.body, this)
        }.build()

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
            assertEquals(runResponse.statusCode, HttpStatus.OK)
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

        Assertions.assertNotEquals(intent, "", "Intent cannot be empty")
        // default intent value is set to "unknown" in AnalyticsInfoContext
        Assertions.assertNotEquals(intent.lowercase(), "unknown", "Intent cannot be \"unknown\"")

        validateRequests(dir)
        logger.debug("finish test case $name")
    }

    protected open fun validateRequests(dir: File) {
    }

    protected fun testData(): Iterable<Arguments> {
        val dir = baseDir
        val resolver = PathMatchingResourcePatternResolver().getResources((Path.of(dir) / "*").toString())
        return resolver
            .map { r: Resource -> r as FileSystemResource }
            .filter { fsr: FileSystemResource -> fsr.file.isDirectory }
            .flatMap { x: FileSystemResource -> resourceToArguments(x) }
            .toList()
    }

    private fun resourceToArguments(x: FileSystemResource): List<Arguments> {
        val filename = x.filename
        val subdirs = x.file.listFiles { file -> file.isDirectory }
        return if (subdirs == null || subdirs.isEmpty()) {
            listOf(Arguments.of(filename, x.path, "/megamind/${scenarioMeta.name}"))
        } else {
            val base = "/" + filename.substring(1)

            subdirs.map { file -> Arguments.of(file.name, file.path, base) }
        }
    }

    protected fun readFiles(dir: File) {
        val dirFiles = dir.listFiles() ?: return
        for (file in dirFiles) {
            files[file.name] = file.readText()
        }
    }

    protected fun readContent(name: String): String? {
        return files[name]
    }

    protected fun <T> readContent(name: String, clazz: Class<T>): T? {
        return readContent(name)?.let { content -> objectMapper.readValue(content, clazz) }
    }

    protected fun <T> readContent(name: String, type: JavaType, mapper: ObjectMapper): T? {
        return readContent(name)?.let { content -> mapper.readValue(content, type) }
    }

    protected fun <T> readList(
        name: String,
        mapper: ObjectMapper,
        elementType: Class<T>
    ): List<T>? {
        return readContent(name)?.let { content ->
            mapper.readValue(content, mapper.typeFactory.constructCollectionType(MutableList::class.java, elementType))
        }
    }

    @AfterEach
    protected open fun tearDown() {
        mockServer.clearAll()
        files.clear()
        requestContext.clear()
    }

    @BeforeEach
    protected open fun setUp() {
        requestContext.clear()
        //TODO: move call to readFiles here
        files.clear()
    }

    protected fun urlForStub(stub: String, path: String): String {
        return "http://localhost:$port/e2e_test/$stub$path"
    }

    protected open fun mockServices(dir: File, uuidReplacement: String, now: Instant) {
        readFiles(dir)
        mockContext()
    }

    protected open fun getHttpHeadersWithJson(): HttpHeaders {
        val headers = HttpHeaders()
        headers.add(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE)
        headers.add(HttpHeaders.ACCEPT, MimeTypeUtils.APPLICATION_JSON_VALUE)
        return headers
    }

    protected open fun getHttpHeaders(): HttpHeaders {
        val headers = HttpHeaders()
        headers.add(X_DEVELOPER_TRUSTED_TOKEN, "TRUSTED")
        headers.add(X_TRUSTED_SERVICE_TVM_CLIENT_ID, TRUSTED_SERVICE_TVM_CLIENT_ID)
        headers.add(X_REQUEST_ID, "C630BCA9-2FF7-4D4E-A18C-805FAC3DA8AC")
        if (requestContext.currentUserId != null) {
            headers.add(SecurityHeaders.USER_TICKET_HEADER, TVM_USER_TICKET_PREFIX + requestContext.currentUserId)
        }
        return headers
    }

    protected open fun validateRequest(stub: String, name: String) {
        val expectedReq = readContent(name, RequestWrapper::class.java)
        if (expectedReq != null) {
            logger.debug("taking request from server: {}", name)
            val requests = mockServer.getRecordedRequests(stub)

            Assertions.assertFalse(requests.isEmpty(), "no requests recorded for $stub")
            logger.debug("taking request from server finished: {}", name)

            val req: RecordedRequest = requests.removeAt(0)

            if (expectedReq.path != null) {
                assertEquals(expectedReq.path, req.path)
            }
            if (expectedReq.method != null) {
                assertEquals(expectedReq.method, req.method)
            }
            if (expectedReq.headers != null) {
                for (key in expectedReq.headers!!.keys) {
                    assertEquals(expectedReq.headers!![key], req.headers!![key])
                }
            }

            if (expectedReq.body != null) {
                val actualRequest = req.readUtf8()
                logger.debug(
                    "actual webhook request:\n{}",
                    objectMapper.readTree(actualRequest).toPrettyString()
                )
                JSONAssert.assertEquals(
                    expectedReq.body,
                    actualRequest, DynamicValueTokenComparator.STRICT
                )
            }
        } else if (name == "webhook_request.json") {
            Assertions.assertTrue(
                mockServer.getRecordedRequests("webhookServer").isEmpty(),
                "no request to webhook expected"
            )
        }
    }

    protected open fun mockContext() {
        val context = readContent("context.json", ContextWrapper::class.java)
        if (context != null) {
            requestContext.currentUserId = context.currentUserId
            requestContext.currentUserTicket = context.currentUserTicket
        }
    }

    protected open fun validateResponse(expectedResponseFile: String, response: ResponseEntity<String>) {
        val expectedRunResponse = Optional.ofNullable(readContent(expectedResponseFile))
            .orElseThrow { RuntimeException("no $expectedResponseFile file") }
        try {
            JSONAssert.assertEquals(
                expectedRunResponse,
                response.body,
                DynamicValueTokenComparator.STRICT
            )
        } catch (e: AssertionError) {
            logger.warn("Assertion failure:" + response.body, e)
            throw e
        }
    }

    @TestConfiguration
    @ContextConfiguration(classes = [MockServer::class])
    @EnableAutoConfiguration(exclude = [JdbcRepositoriesAutoConfiguration::class, DataSourceAutoConfiguration::class])
    open class BaseIntegrationTestConfiguration {
        @Autowired
        private val env: Environment? = null

        @Bean("tvmClient")
        @Primary
        open fun tvmClient(): MockTvmClient {
            return MockTvmClient()
        }

        @Bean("versionProvider")
        open fun versionProvider(): VersionProvider {
            return TestVersionProvider()
        }

        @Bean
        open fun localPortCustomizer(): RestTemplateCustomizer {
            return RestTemplateCustomizer { restTemplate -> restTemplate.interceptors.add(localPortInterceptor()) }
        }

        fun localPortInterceptor(): ClientHttpRequestInterceptor {
            return ClientHttpRequestInterceptor { request, body, execution ->
                if (request.uri.toString().contains("localhost/e2e_test/")) {
                    val serverPort = env!!.getProperty("local.server.port")
                        ?: throw RuntimeException("local.server.port property not found")

                    val origUri = request.uri
                    val requestWithFixedPort: HttpRequest = object : HttpRequestWrapper(request) {
                        override fun getURI(): URI {
                            return UriComponentsBuilder.fromUri(origUri)
                                .port(serverPort.toInt())
                                .build()
                                .toUri()
                        }
                    }
                    return@ClientHttpRequestInterceptor execution.execute(requestWithFixedPort, body)
                }
                execution.execute(request, body)
            }
        }
    }

    class MockTvmClient : TvmClient {
        override fun getStatus(): ClientStatus {
            return STATUS
        }

        override fun getServiceTicketFor(alias: String): String {
            return "xxx-ticket"
        }

        override fun getServiceTicketFor(tvmId: Int): String {
            return "xxx-ticket"
        }

        override fun checkServiceTicket(ticketBody: String): CheckedServiceTicket {
            return Unittest.createServiceTicket(TicketStatus.OK, 1)
        }

        override fun checkUserTicket(ticket: String): CheckedUserTicket {
            return if (ticket.startsWith(TVM_USER_TICKET_PREFIX)) {
                Unittest.createUserTicket(
                    TicketStatus.OK,
                    ticket.substring(TVM_USER_TICKET_PREFIX.length).toLong(),
                    arrayOfNulls(0),
                    LongArray(0)
                )
            } else {
                Unittest.createUserTicket(TicketStatus.EXPIRED, 1, arrayOfNulls(0), LongArray(0))
            }
        }

        override fun checkUserTicket(ticketBody: String, overridedBbEnv: BlackboxEnv): CheckedUserTicket {
            return checkUserTicket(ticketBody)
        }

        override fun getRoles(): Roles =
            throw UnsupportedOperationException("Not implemented")

        override fun close() {}

        companion object {
            private val STATUS = ClientStatus(ClientStatus.Code.OK, "")
        }
    }

    protected class TestVersionProvider : VersionProvider {
        @get:Nonnull
        override val version: String
            get() = "100"

        @get:Nonnull
        override val branch: String
            get() = "unknown-vcs-branch"

        @get:Nonnull
        override val tag: String
            get() = ""
    }

    @RestController
    class MockServer {
        private val stubResponses: MutableMap<String, Queue<MockResponse>> = ConcurrentHashMap()
        private val recordedRequests: MutableMap<String, MutableList<RecordedRequest>> = ConcurrentHashMap()

        @RequestMapping(path = ["/e2e_test/{stub}/**", "/e2e_test/{stub}"])
        fun stub(
            request: HttpServletRequest,
            @PathVariable("stub") stub: String,
            @RequestBody(required = false) body: ByteArray?
        ): ResponseEntity<String?> {
            val headers: LinkedCaseInsensitiveMap<String> = request.headerNames.toList()
                .associateWithTo(LinkedCaseInsensitiveMap()) { name -> request.getHeader(name) }
            recordedRequests.computeIfAbsent(stub) { arrayListOf() }
                .add(
                    RecordedRequest(
                        headers,
                        request.servletPath.replace("/e2e_test/$stub", ""),
                        request.method,
                        body
                    )
                )
            val stubbedResponses = stubResponses[stub]
            return if (stubbedResponses != null) {
                stubbedResponses.poll()
                    .takeIf { response -> response.responseBody != null }
                    ?.also { response ->
                        if (response.delayed) {
                            Thread.sleep(MOCK_RESPONSE_DELAY)
                        }
                    }
                    ?.let { response ->
                        ResponseEntity.ok().contentType(MediaType.APPLICATION_JSON).body(response.responseBody)
                    }
                    ?: ResponseEntity.status(500).build()
            } else {
                ResponseEntity.status(500).build()
            }
        }

        fun clearAll() {
            stubResponses.clear()
            recordedRequests.clear()
        }

        fun getRecordedRequests(stub: String): MutableList<RecordedRequest> {
            return recordedRequests.getOrDefault(stub, mutableListOf())
        }

        fun enqueueResponse(stub: String, vararg responses: String) {
            stubResponses.computeIfAbsent(stub) { LinkedList() }
                .addAll(
                    Arrays.stream(responses).map { responseBody: String? -> MockResponse.mockResponse(responseBody) }
                        .collect(Collectors.toList()))
        }

        fun enqueueDelayedResponse(stub: String, vararg responses: String) {
            stubResponses.computeIfAbsent(stub) { LinkedList() }
                .addAll(
                    Arrays.stream(responses)
                        .map { responseBody: String? -> MockResponse.mockDelayedResponse(responseBody) }
                        .collect(Collectors.toList()))
        }

        internal class MockResponse(var responseBody: String?, var delayed: Boolean) {
            companion object {
                fun mockResponse(responseBody: String?): MockResponse {
                    return MockResponse(responseBody, false)
                }

                fun mockDelayedResponse(responseBody: String?): MockResponse {
                    return MockResponse(responseBody, true)
                }
            }
        }
    }

    protected data class MockServerRequest(
        val method: HttpMethod,
        val uri: URI,
        val headers: HttpHeaders,
        val body: String,
    )

    protected data class ContextWrapper(
        var currentUserId: String?,
        var currentUserTicket: String?,
    )

    protected data class RequestWrapper(
        var headers: LinkedCaseInsensitiveMap<String>?,
        var path: String?,
        var method: String?,

        @JsonDeserialize(using = AnythingToStringJacksonDeserializer::class)
        var body: String? = null,
    )

    class RecordedRequest(
        val headers: LinkedCaseInsensitiveMap<String>?,
        val path: String?,
        val method: String?,
        val body: ByteArray?,
    ) {
        fun readUtf8(): String {
            return body?.let { String(it, StandardCharsets.UTF_8) } ?: ""
        }
    }

    companion object {
        protected const val TRUSTED_SERVICE_TVM_CLIENT_ID = "2000860"
        protected const val TVM_USER_TICKET_PREFIX = "TVM-USER-"
        protected const val MOCK_RESPONSE_DELAY: Long = 4000

        private val logger = LogManager.getLogger()
    }
}


