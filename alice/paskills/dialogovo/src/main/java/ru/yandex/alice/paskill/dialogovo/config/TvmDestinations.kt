package ru.yandex.alice.paskill.dialogovo.config

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.springframework.beans.factory.annotation.Value
import org.springframework.core.io.support.PathMatchingResourcePatternResolver
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.tvm.TvmDestinationRegistrar

@Component
class TvmDestinations(
    @Value("\${spring.profiles.active}") profile: String,
    objectMapper: ObjectMapper
) : TvmDestinationRegistrar {
    private val destinations: Map<String, Int>

    init {
        val resource = PathMatchingResourcePatternResolver()
            .getResource("classpath:config/dialogovo-tvm-${profile}.json")

        this.destinations = resource.inputStream.use { objectMapper.readValue(it) }
    }

    override fun register(aliases: MutableMap<String, Int>) {
        aliases.putAll(destinations)
    }
}
