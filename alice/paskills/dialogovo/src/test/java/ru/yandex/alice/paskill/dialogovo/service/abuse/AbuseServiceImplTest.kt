package ru.yandex.alice.paskill.dialogovo.service.abuse

import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.http.HttpHeaders
import org.springframework.util.MimeTypeUtils
import org.springframework.web.client.RestTemplate
import java.io.IOException
import java.nio.file.Files

internal class AbuseServiceImplTest {
    private lateinit var abuseService: AbuseServiceImpl
    private lateinit var server: MockWebServer

    private val patternResolver = PathMatchingResourcePatternResolver()

    @BeforeEach
    fun setUp() {
        server = MockWebServer()
        server.start()
        abuseService = AbuseServiceImpl(RestTemplate(), abuseUrl = server.url("/").toString(), cacheSize = 100)
    }

    @AfterEach
    fun tearDown() {
        server.shutdown()
    }

    @Test
    fun singleText() {
        server.enqueue(
            MockResponse()
                .setBody(readFile("abuse_client/one/abuse_response.json"))
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE)
        )
        val abused = abuseService.checkAbuse(
            listOf(
                DataAbuseDocument("response.text", "блять ты че совсем охуел"),
                DataAbuseDocument("response.tts", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.title", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.description", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.button.text", "блять ты че совсем охуел")
            )
        )
        Assertions.assertEquals(
            mapOf(
                "response.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.tts" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.card.title" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.card.description" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.card.button.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>"
            ),
            abused
        )
    }

    @Test
    fun abuseFailure() {
        server.enqueue(MockResponse().setResponseCode(500))
        val abused = abuseService.checkAbuse(
            listOf(
                DataAbuseDocument("response.text", "блять ты че совсем охуел"),
                DataAbuseDocument("response.tts", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.title", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.description", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.button.text", "блять ты че совсем охуел")
            )
        )
        Assertions.assertEquals(
            mapOf(
                "response.text" to "блять ты че совсем охуел",
                "response.tts" to "блять ты че совсем охуел",
                "response.card.title" to "блять ты че совсем охуел",
                "response.card.description" to "блять ты че совсем охуел",
                "response.card.button.text" to "блять ты че совсем охуел"
            ),
            abused
        )
    }

    @Test
    fun twoTexts() {
        server.enqueue(
            MockResponse()
                .setBody(readFile("abuse_client/two/abuse_response.json"))
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE)
        )
        val abused = abuseService.checkAbuse(
            listOf(
                DataAbuseDocument("response.text", "блять ты че совсем охуел"),
                DataAbuseDocument("response.tts", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.title", "ну ты пидор"),
                DataAbuseDocument("response.card.description", "ну ты пидор"),
                DataAbuseDocument("response.card.button.text", "блять ты че совсем охуел")
            )
        )
        Assertions.assertEquals(
            mapOf(
                "response.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.tts" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
                "response.card.title" to "ну ты <censored>пидор</censored>",
                "response.card.description" to "ну ты <censored>пидор</censored>",
                "response.card.button.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>"
            ),
            abused
        )
    }

    @Test
    @Throws(IOException::class)
    fun fetchFromCache() {
        server.enqueue(
            MockResponse()
                .setBody(readFile("abuse_client/two/abuse_response.json"))
                .setHeader(HttpHeaders.CONTENT_TYPE, MimeTypeUtils.APPLICATION_JSON_VALUE)
        )

        // second attempt files
        server.enqueue(MockResponse().setResponseCode(500))
        val actual = abuseService.checkAbuse(
            listOf(
                DataAbuseDocument("response.text", "блять ты че совсем охуел"),
                DataAbuseDocument("response.tts", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.title", "ну ты пидор"),
                DataAbuseDocument("response.card.description", "ну ты пидор"),
                DataAbuseDocument("response.card.button.text", "блять ты че совсем охуел")
            )
        )
        val expected = mapOf(
            "response.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
            "response.tts" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>",
            "response.card.title" to "ну ты <censored>пидор</censored>",
            "response.card.description" to "ну ты <censored>пидор</censored>",
            "response.card.button.text" to "<censored>блять</censored> ты че совсем <censored>охуел</censored>"
        )
        Assertions.assertEquals(expected, actual)

        // would fail on cache miss
        val actual2 = abuseService.checkAbuse(
            listOf(
                DataAbuseDocument("response.text", "блять ты че совсем охуел"),
                DataAbuseDocument("response.tts", "блять ты че совсем охуел"),
                DataAbuseDocument("response.card.title", "ну ты пидор"),
                DataAbuseDocument("response.card.description", "ну ты пидор"),
                DataAbuseDocument("response.card.button.text", "блять ты че совсем охуел")
            )
        )
        Assertions.assertEquals(expected, actual2)
    }

    private fun readFile(path: String): String {
        return Files.readString(patternResolver.getResources(path)[0].file.toPath())
    }

    private data class DataAbuseDocument(
        override val id: String,
        override val value: String,
    ) : AbuseDocument
}
