package ru.yandex.alice.kronstadt.scenarios.tvmain.serializers

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import org.assertj.core.api.Assertions.assertThat
import org.junit.jupiter.api.Test
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import ru.yandex.alice.kronstadt.scenarios.tvmain.VhCarousel


class VhDocumentDeserializerTest {

    private val objectMapper = jacksonObjectMapper().copy()
        .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)

    @Test
    fun testComplexAnswer() {
        val resource = PathMatchingResourcePatternResolver()
            .getResource("classpath:serializers/vhCarousel.json")

        val parsedCarousel: VhCarousel = objectMapper
            .readValue(resource.inputStream.readAllBytes(), VhCarousel::class.java)
        assertThat(parsedCarousel.set.size).isEqualTo(10)
        assertThat(parsedCarousel.items().size).isEqualTo(8)
    }
}
