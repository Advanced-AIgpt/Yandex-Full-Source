package ru.yandex.alice.library.protobufutils

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.Struct

/**
 * This proxy is needed to expose [JacksonToProtoConverter] internal methods to java for benchmark
 */
class JacksonToProtoConverterProxy {
    private val mapper = jacksonObjectMapper().copy().disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)
    val protoUtil = JacksonToProtoConverter(mapper)

    inline fun <reified T> structToObject(source: Struct): T = protoUtil.structToObject(source)

    fun <T> structToObject(source: Struct, clazz: Class<T>): T = protoUtil.structToObject(source, clazz)

    fun <T> structToObjectViaJsonString(source: Struct, clazz: Class<T>): T =
        protoUtil.structToObjectViaJsonString(source, clazz)

    fun <T> structToObjectViaTree(source: Struct, clazz: Class<T>): T = protoUtil.structToObjectViaTree(source, clazz)

    fun structToObjectNode(source: Struct): ObjectNode = protoUtil.structToObjectNode(source)

    fun structToMap(source: Struct): Map<String, Any?> = protoUtil.structToMap(source)

    fun structToMapViaObjectMapper(source: Struct): Map<String, Any?> = protoUtil.structToMapViaObjectMapper(source)

    fun structToMapDirect(source: Struct): Map<String, Any?> = protoUtil.structToMapDirect(source)

    fun objectToStruct(source: Any?) = protoUtil.objectToStruct(source)

    fun objectToStructViaTree(source: Any): Struct = protoUtil.objectToStructViaTree(source)

    fun objectToStructViaString(source: Any): Struct = protoUtil.objectToStructViaString(source)
}
