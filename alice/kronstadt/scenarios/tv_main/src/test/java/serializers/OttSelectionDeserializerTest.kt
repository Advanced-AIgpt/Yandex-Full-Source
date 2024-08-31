package ru.yandex.alice.kronstadt.scenarios.tvmain.serializers

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import org.junit.jupiter.api.Test
import org.assertj.core.api.Assertions.assertThat
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import ru.yandex.alice.kronstadt.scenarios.tvmain.ShowcaseGatewayDto

class OttSelectionDeserializerTest {
    private val objectMapper = jacksonObjectMapper().copy()
        .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)

    @Test
    fun testComplexAnswer() {
        val resource = PathMatchingResourcePatternResolver()
            .getResource("classpath:serializers/ott-selections.json")

        val parsedCarousel: ShowcaseGatewayDto = objectMapper
            .readValue(resource.inputStream.readAllBytes(), ShowcaseGatewayDto::class.java)
        assertThat(parsedCarousel.collections.size).isEqualTo(12)
        for (collection in parsedCarousel.collections) {
            assertThat(collection.data.size)
                .isEqualTo(if (collection.selectionId == "editorial_feature__30") 7 else 10)
        }
    }
}
