package ru.yandex.alice.kronstadt.core.utils

import com.fasterxml.jackson.core.JsonGenerator
import com.fasterxml.jackson.databind.JsonSerializer
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.SerializerProvider
import java.io.IOException

class AnythingFromStringJacksonSerializer : JsonSerializer<String>() {

    @Throws(IOException::class)
    override fun serialize(value: String, gen: JsonGenerator, serializers: SerializerProvider) {
        val mapper = ObjectMapper()
        val tree = mapper.readTree(value)
        gen.writeObject(tree)
    }
}
