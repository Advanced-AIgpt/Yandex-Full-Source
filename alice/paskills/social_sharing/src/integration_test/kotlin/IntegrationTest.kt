package ru.yandex.alice.social.sharing

import ApphostFixture
import NAlice.NNotificator.Api
import NMatrix.NApi.Delivery
import NAppHostHttp.Http
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ArrayNode
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.databind.node.ValueNode
import com.fasterxml.jackson.module.kotlin.readValue
import com.google.common.io.Files
import com.google.protobuf.ByteString
import com.google.protobuf.Message
import com.google.protobuf.util.JsonFormat
import createApphostFixture
import org.apache.http.client.utils.URIBuilder
import org.apache.logging.log4j.Level
import org.apache.logging.log4j.LogManager
import org.apache.logging.log4j.core.config.Configurator
import org.apache.logging.log4j.core.config.DefaultConfiguration
import org.junit.jupiter.api.AfterAll
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.BeforeAll
import org.junit.jupiter.params.ParameterizedTest
import org.junit.jupiter.params.provider.Arguments
import org.junit.jupiter.params.provider.MethodSource
import org.skyscreamer.jsonassert.JSONAssert
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Qualifier
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.boot.test.context.TestConfiguration
import org.springframework.context.annotation.Bean
import org.springframework.core.io.FileSystemResource
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.http.HttpEntity
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.http.ResponseEntity
import org.springframework.test.context.ActiveProfiles
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig
import org.springframework.web.client.HttpStatusCodeException
import org.springframework.web.client.RestTemplate
import org.springframework.web.util.UriComponentsBuilder
import ru.yandex.alice.apphost.MockHttpNode
import ru.yandex.alice.apphost.SingleResponseHttpHandler
import ru.yandex.alice.social.sharing.apphost.handlers.TemplateHandler
import ru.yandex.alice.social.sharing.document.Document
import ru.yandex.alice.social.sharing.document.Image
import ru.yandex.alice.social.sharing.proto.WebPageProto
import ru.yandex.web.apphost.api.AppHostService
import java.io.File
import java.lang.Exception
import java.lang.RuntimeException
import java.net.URI

fun interface MockBodyConverter {
    fun convert(mockResponse: String): ByteString
}

val idConverter = MockBodyConverter {
    ByteString.copyFrom(it.toByteArray())
}

private class ProtoMockBodyConverter(
    private val protoBuilder: Message.Builder
): MockBodyConverter {
    override fun convert(mockResponse: String): ByteString {
        JsonFormat.parser().merge(mockResponse, protoBuilder)
        return protoBuilder.build().toByteString()
    }
}

private val ResponseEntity<String>.bodyWithoutEventlog: String?
    get() = this.body?.replace(Regex("//DEBUGINFO[\\w\\W]*", RegexOption.MULTILINE), "")

private val ResponseEntity<String>.eventlog: String?
    get() {
        val eventlogStart = this.body?.indexOf("//DEBUGINFO")
        return if (eventlogStart != null && eventlogStart >= 0) {
            this.body?.substring(eventlogStart)
        } else {
            null
        }
    }

private fun srcrwr(node: String, port: Int, timeout: Long = 1000000000): String {
    return "$node:localhost:$port:$timeout"
}

@SpringBootTest(
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
@SpringJUnitConfig(
    classes = [
        IntegrationTest.TemplateConfiguration::class,
        IntegrationTest.DocumentProviderConfiguration::class,
        IntegrationTest.ImageProviderConfiguration::class,
    ]
)
@ActiveProfiles("it")
class IntegrationTest {

    private val REQUEST_FILENAME = "REQUEST.json"
    private val REQUEST_BODY_FILENAME = "REQUEST_body.json"
    private val RESPONSE_FILENAME = "RESPONSE.json"
    private val RESPONSE_BODY_FILENAME = "RESPONSE_body.json"

    private val APP_BACKENDS: List<String> = listOf(
        "ALICE_SOCIAL_SHARE",
    )

    @Autowired
    @Qualifier("appHostService")
    private lateinit var appHostService: AppHostService

    @Autowired
    @Qualifier("mockTemplateApphostService")
    private lateinit var mockTemplateApphostService: AppHostService

    @Autowired
    private lateinit var documentProvider: TestDocumentProvider

    @Autowired
    private lateinit var imageProvider: TestImageProvider

    @Autowired
    private lateinit var objectMapper: ObjectMapper

    private val restTemplate: RestTemplate = RestTemplate()

    private val mockHttpNodes: MutableMap<String, MockHttpNode> = mutableMapOf()

    @ParameterizedTest(name = "{0}")
    @MethodSource("getTestData")
    fun test(name: String, path: String) {
        val dir = File(path)
        prepareDocuments(dir)
        prepareImages(dir)
        createMocks(dir)
        val request: Http.THttpRequest = parseRequest(dir)
        val expectedResponse: Http.THttpResponse = parseResponse(dir)
        val requestUri = URI(request.path)
        val httpAdapterUriBuilder = URIBuilder()
            .setScheme("http")
            .setHost("localhost")
            .setPort(apphost.httpAdapterPort)
            .setPath(requestUri.path)
            .setParameter("dump", "eventlog")
            .setParameter("timeout", "1000000000")
            .setParameter("waitall", "da")
        if (!requestUri.query.isNullOrEmpty()) {
            for (queryParamString in requestUri.query.split("&")) {
                val parts = queryParamString.split("=", limit = 2)
                val key = parts[0]
                val value = if (parts.size == 2) parts[1] else ""
                httpAdapterUriBuilder.setParameter(key, value)
            }
        }
        APP_BACKENDS.forEach { node ->
            httpAdapterUriBuilder.addParameter("srcrwr", srcrwr(node, appHostService.port))
        }
        httpAdapterUriBuilder.addParameter("srcrwr", srcrwr("TEMPLATES", mockTemplateApphostService.port))
        mockHttpNodes.forEach {
            httpAdapterUriBuilder.addParameter("srcrwr", srcrwr(it.key, it.value.port))
        }
        val headers = HttpHeaders()
        headers.add("X-Yandex-Internal-Request", "1")
        request.headersList.forEach { headers.add(it.name, it.value) }
        val httpEntity = HttpEntity(request.content.toStringUtf8(), headers)
        var response: ResponseEntity<String>? = null
        try {
            val url = httpAdapterUriBuilder.build()
            println("Sending request to $url")
            val method = when (request.method) {
                Http.THttpRequest.EMethod.Get -> HttpMethod.GET
                Http.THttpRequest.EMethod.Post -> HttpMethod.POST
                else -> throw RuntimeException("Unsupported THTTPRequest.EMethod: ${request.method}")
            }
            response = restTemplate.exchange(
                url,
                method,
                httpEntity,
                String::class.java,
            )
            println("""HTTP adapter response:
                |${response.statusCode} ${response.statusCodeValue}
                |${response.headers.map{ it.key + ": " + it.value }.joinToString("\n")}
                |
                |${response.bodyWithoutEventlog}""".trimMargin())
            Assertions.assertEquals(expectedResponse.statusCode, response.statusCode.value())

            if (!expectedResponse.content.toStringUtf8().isNullOrEmpty()) {
                JSONAssert.assertEquals(expectedResponse.content.toStringUtf8(), response.bodyWithoutEventlog, true)
            } else {
                Assertions.assertEquals(response.bodyWithoutEventlog?.trim(), "")
            }

            for (header in expectedResponse.headersList) {
                val name = header.name
                val expectedValue = header.value
                assertTrue(response.headers.containsKey(name), "Missing response header: ${header}")
                val headerValues = response.headers.get(name)!!
                if (name.lowercase() == "location") {
                    val headerValuesWithoutApphostParams = headerValues
                        .map { it.replace("(%26)?srcrwr.*".toRegex(), "") }
                    assertTrue(
                        headerValuesWithoutApphostParams.any { it == expectedValue },
                        "Invalid response header $name: expected \"$expectedValue\", got $headerValuesWithoutApphostParams"
                    )
                } else {
                    assertTrue(
                        headerValues.any { it == expectedValue },
                        "Invalid response header $name: expected \"$expectedValue\", got $headerValues"
                    )
                }
            }
        } catch (e: HttpStatusCodeException) {
            if (e.statusCode.value() >= 500) {
                logger.error(
                    "Apphost request failed with status {} and body\n{}",
                    e.statusCode,
                    e.responseBodyAsString,
                )
                throw e
            }
            Assertions.assertEquals(expectedResponse.statusCode, e.statusCode)
            JSONAssert.assertEquals(expectedResponse.content.toStringUtf8(), e.responseBodyAsString, true)
        } finally {
            println("apphost eventlog:\n${response?.eventlog}")
        }
    }

    private fun parseRequest(dir: File) : Http.THttpRequest {
        val requestFile = File(dir.path + File.separator + REQUEST_FILENAME)
        Assertions.assertTrue(requestFile.exists())
        val content = readFile(requestFile)
        val httpRequestBuilder = Http.THttpRequest.newBuilder()
        JsonFormat.parser().merge(content, httpRequestBuilder)
        val requestBodyFile = File(dir.path + File.separator + REQUEST_BODY_FILENAME)
        if (requestBodyFile.exists() && requestBodyFile.isFile) {
            val content = readFile(requestBodyFile)
            httpRequestBuilder.setContent(ByteString.copyFrom(content, UTF_8))
            logger.info("Request body in THTTPRequest: {}", content)
        }
        return httpRequestBuilder.build()
    }

    private fun parseResponse(dir: File) : Http.THttpResponse {
        val responseFile = File(dir.path + File.separator + RESPONSE_FILENAME)
        Assertions.assertTrue(responseFile.exists(), "RESPONSE.json doesn't exist")
        val content = readFile(responseFile)
        val httpResponseBuilder = Http.THttpResponse.newBuilder()
        JsonFormat.parser().merge(content, httpResponseBuilder)
        val responseBodyFile = File(dir.path + File.separator + RESPONSE_BODY_FILENAME)
        if (responseBodyFile.exists()) {
            httpResponseBuilder.setContent(ByteString.copyFrom(readFile(responseBodyFile).toByteArray()))
        }
        return httpResponseBuilder.build()
    }

    private fun prepareDocuments(dir: File) {
        documentProvider.clear()
        val documentsFile = File(dir.path + File.separator + "documents.json")
        if (documentsFile.exists()) {
            val documents = objectMapper.readTree(readFile(documentsFile))
            Assertions.assertTrue(documents.isArray)
            for (jsonDocument in documents as ArrayNode) {
                val id: String = (jsonDocument["id"] as ValueNode).asText()
                val pageJson = jsonDocument["page"] as ObjectNode
                val pageBuilder = WebPageProto.TScenarioSharePage.newBuilder()
                JsonFormat.parser().merge(objectMapper.writeValueAsString(pageJson), pageBuilder)
                val document = Document(
                    id,
                    pageBuilder.build()
                )
                documentProvider.createCandidate(document)
                documentProvider.commitCandidate(document.id)
            }
        }
    }

    private fun prepareImages(dir: File) {
        imageProvider.clear()
        val imagesFile = File(dir.path + File.separator + "images.json")
        if (imagesFile.exists()) {
            val images = objectMapper.readValue<List<Image>>(readFile(imagesFile))
            for (image in images) {
                imageProvider.upsert(image, "")
            }
        }
    }

    private fun createMocks(dir: File) {
        createSingleMock(dir, "BLACKBOX")
        createSingleMock(dir, "IOT_DEVICE_LIST")
        createSingleMock(
            dir,
            "NOTIFICATOR_DEVICE_LIST",
            ProtoMockBodyConverter(Api.TGetDevicesResponse.newBuilder()),
        )
        createSingleMock(
            dir,
            "NOTIFICATOR_SEND_PUSH",
            ProtoMockBodyConverter(Delivery.TDeliveryResponse.newBuilder()),
        )
        createSingleMock(dir, "GET_SKILL_INFO")
        createSingleMock(dir, "GET_SKILL_SECRET")
        createSingleMock(dir, "AVATARS")
    }

    private fun createSingleMock(
        dir: File,
        apphostNode: String,
        bodyConverter: MockBodyConverter = idConverter,
    ) {
        val httpRequestFile = File(dir.path + File.separator + apphostNode + ".json")
        val httpResponse = Http.THttpResponse.newBuilder()
        if (httpRequestFile.exists()) {
            JsonFormat.parser().merge(readFile(httpRequestFile), httpResponse)
        } else {
            httpResponse.statusCode = 200
        }
        val requestBodyFile = File(dir.path + File.separator + apphostNode + "_body.json")
        if (requestBodyFile.exists()) {
            httpResponse.content = bodyConverter.convert(readFile(requestBodyFile))
        }
        mockHttpNodes[apphostNode] = MockHttpNode(
            SingleResponseHttpHandler(httpResponse.build())
        )
    }

    private fun readFile(file: File): String = Files.asCharSource(file, UTF_8).read()

    companion object {

        private val logger = LogManager.getLogger()

        private lateinit var apphost: ApphostFixture

        @BeforeAll
        @JvmStatic
        fun setup() {
            Configurator.initialize(DefaultConfiguration())
            Configurator.setRootLevel(Level.INFO)
            apphost = createApphostFixture()
        }

        @AfterAll
        @JvmStatic
        fun teardown() {
            apphost.close()
        }

        @JvmStatic
        fun getTestData(): List<Arguments> {
            return PathMatchingResourcePatternResolver()
                .getResources("mocks/*")
                .map { it as FileSystemResource }
                .map { Arguments.of(it.filename, it.path, ) }
        }
    }

    @TestConfiguration
    open class TemplateConfiguration {
        @Bean("templateHandler")
        open fun templateHandler(objectMapper: ObjectMapper): TemplateHandler {
            return MockTemplates(objectMapper)
        }
    }

    @TestConfiguration
    open class DocumentProviderConfiguration {
        @Bean("documentProvider")
        open fun documentProvider(): TestDocumentProvider {
            return InMemoryDocumentProvider()
        }
    }

    @TestConfiguration
    open class ImageProviderConfiguration {
        @Bean("imageProvider")
        open fun imageProvider(): TestImageProvider {
            return InMemoryImageProvider()
        }
    }

}
