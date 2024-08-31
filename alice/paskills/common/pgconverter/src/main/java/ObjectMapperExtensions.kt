package ru.yandex.alice.paskills.common.pgconverter

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.readValue
import org.postgresql.util.PGobject

fun <T> ObjectMapper.toPgObject(source: T, type: String = "json"): PGobject {
    val obj = PGobject()
    obj.type = type
    obj.value = this.writeValueAsString(source)
    return obj
}

inline fun <reified T> ObjectMapper.readPgObject(src: PGobject): T? = src.value?.let { this.readValue<T>(it) }
