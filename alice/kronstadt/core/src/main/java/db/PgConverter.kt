package ru.yandex.alice.kronstadt.core.db

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.databind.ObjectMapper
import org.springframework.core.convert.converter.Converter
import java.io.IOException

abstract class PgConverter<From, To>(
    private val objectMapper: ObjectMapper,
): Converter<From, To> {
    protected fun <T> jsonToString(source: T?): String? {
        return if (source == null) {
            null
        } else try {
            objectMapper.writeValueAsString(source)
        } catch (e: JsonProcessingException) {
            throw RuntimeException(e)
        }
    }

    protected fun <T> stringToJsonObject(source: String?, clazz: Class<T>): T? {
        return if (source == null || source.isEmpty()) {
            null
        } else try {
            objectMapper.readValue(source, clazz)
        } catch (e: IOException) {
            throw RuntimeException(e)
        }
    }

}
