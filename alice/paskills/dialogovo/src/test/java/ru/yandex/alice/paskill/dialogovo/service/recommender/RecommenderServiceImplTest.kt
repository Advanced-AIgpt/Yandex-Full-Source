package ru.yandex.alice.paskill.dialogovo.service.recommender

import com.fasterxml.jackson.databind.ObjectMapper
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.boot.autoconfigure.EnableAutoConfiguration
import org.springframework.boot.test.context.SpringBootTest
import org.springframework.context.annotation.Configuration
import org.springframework.web.client.RestTemplate
import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider
import ru.yandex.alice.paskill.dialogovo.test.TestSkill
import ru.yandex.alice.paskill.dialogovo.test.TestSkills
import ru.yandex.alice.paskill.dialogovo.utils.ResourceUtils
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.IOException

@SpringBootTest(
    classes = [TestConfigProvider::class, RecommenderServiceImplTest.TestConfiguration::class, TestSkill::class],
    webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
internal class RecommenderServiceImplTest {
    private lateinit var mockRecommenderServer: MockWebServer
    private lateinit var recommenderService: RecommenderServiceImpl

    @Autowired
    private lateinit var testSkill: TestSkill

    @Autowired
    private lateinit var objectMapper: ObjectMapper

    @BeforeEach
    fun setUp() {
        mockRecommenderServer = MockWebServer()
        mockRecommenderServer.start()
        recommenderService =
            RecommenderServiceImpl(RestTemplate(), MetricRegistry.root(), mockRecommenderServer.url("/").toString())
    }

    @AfterEach
    fun tearDown() {
        mockRecommenderServer.shutdown()
    }

    @Throws(IOException::class)
    private fun mockServerResponse(recommenderResponseFilename: String) {
        val recommenderResponseBody =
            ResourceUtils.getStringResource("recommender_responses/$recommenderResponseFilename")
        val recommenderResponse = MockResponse()
            .addHeader("Content-Type: application/json;charset=UTF-8")
            .setBody(recommenderResponseBody)
        mockRecommenderServer.enqueue(recommenderResponse)
    }

    @Test
    @Throws(IOException::class)
    fun testSearchOneResult() {
        mockServerResponse("one_result.json")
        val response = recommenderService.search(
            RecommenderCardName.DISCOVERY_BASS_SEARCH, "города",
            RecommenderRequestAttributes()
        )
        assertEquals(response.items.size.toLong(), 1)
        assertEquals(response.items.first().skillId, TestSkills.cityGameSkill().id)
    }

    @Test
    @Throws(IOException::class)
    fun testNoResults() {
        mockServerResponse("no_results.json")
        val response = recommenderService.search(
            RecommenderCardName.DISCOVERY_BASS_SEARCH, "города",
            RecommenderRequestAttributes()
        )
        assertTrue(response.items.isEmpty())
    }

    @Configuration
    @EnableAutoConfiguration
    internal open class TestConfiguration
}
