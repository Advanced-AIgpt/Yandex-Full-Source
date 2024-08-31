package ru.yandex.alice.library.protobufutils

import com.fasterxml.jackson.core.JsonProcessingException
import com.fasterxml.jackson.core.type.TypeReference
import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ArrayNode
import com.fasterxml.jackson.databind.node.BooleanNode
import com.fasterxml.jackson.databind.node.JsonNodeFactory
import com.fasterxml.jackson.databind.node.NullNode
import com.fasterxml.jackson.databind.node.NumericNode
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.databind.node.TextNode
import com.google.protobuf.InvalidProtocolBufferException
import com.google.protobuf.ListValue
import com.google.protobuf.Struct
import com.google.protobuf.Value
import com.google.protobuf.Value.KindCase
import com.google.protobuf.util.JsonFormat
import com.google.protobuf.util.Values

class JacksonToProtoConverter(private val objectMapper: ObjectMapper) {

    private val nodeFactory: JsonNodeFactory = objectMapper.nodeFactory
    private val jsonPrinter: JsonFormat.Printer = JsonFormat.printer()
    private val jsonParse: JsonFormat.Parser = JsonFormat.parser()

    init {
        if (objectMapper.isEnabled(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES)) {
            throw RuntimeException(
                "\"FAIL_ON_UNKNOWN_PROPERTIES\" deserialization feature must be disabled " +
                    "in ObjectMapper for protobuf conversions"
            )
        }
    }

    fun jsonStringToStruct(source: String?): Struct {
        if (source == null) {
            return Struct.getDefaultInstance()
        }
        val struct = Struct.newBuilder()
        return try {
            jsonParse.merge(source, struct)
            if (struct.fieldsCount == 0) {
                Struct.getDefaultInstance()
            } else {
                struct.build()
            }
        } catch (e: InvalidProtocolBufferException) {
            throw RuntimeException(e)
        }
    }

    inline fun <reified T> structToObject(source: Struct): T = structToObject(source, T::class.java)

    fun <T> structToObject(source: Struct, clazz: Class<T>): T {
        //return structToObjectViaJsonString(source, clazz)
        return structToObjectViaTree(source, clazz)
    }

    internal fun <T> structToObjectViaJsonString(source: Struct, clazz: Class<T>): T = try {
        val json = jsonPrinter.print(source)
        objectMapper.readValue(json, clazz)
    } catch (e: InvalidProtocolBufferException) {
        throw RuntimeException(e)
    } catch (e: JsonProcessingException) {
        throw RuntimeException(e)
    }

    internal fun <T> structToObjectViaTree(source: Struct, clazz: Class<T>): T = try {
        val obj = structToObjectNode(source)
        objectMapper.treeToValue(obj, clazz)
    } catch (e: JsonProcessingException) {
        throw RuntimeException(e)
    }

    internal fun structToObjectNode(source: Struct): ObjectNode {
        val obj = nodeFactory.objectNode()
        source.fieldsMap.forEach { (key, value) ->
            obj.set<ObjectNode>(key, valueToJsonNode(value))
        }
        return obj
    }

    private fun listValueToJsonArray(list: ListValue): ArrayNode {
        val arr = nodeFactory.arrayNode(list.valuesCount)
        list.valuesList.forEach { arr.add(valueToJsonNode(it)) }
        return arr
    }

    @Suppress("WHEN_ENUM_CAN_BE_NULL_IN_JAVA")
    private fun valueToJsonNode(value: Value): JsonNode = when (value.kindCase) {
        KindCase.NULL_VALUE -> nodeFactory.nullNode()
        KindCase.NUMBER_VALUE -> nodeFactory.numberNode(value.numberValue)
        KindCase.STRING_VALUE -> nodeFactory.textNode(value.stringValue)
        KindCase.BOOL_VALUE -> nodeFactory.booleanNode(value.boolValue)
        KindCase.STRUCT_VALUE -> structToObjectNode(value.structValue)
        KindCase.LIST_VALUE -> listValueToJsonArray(value.listValue)
        KindCase.KIND_NOT_SET -> nodeFactory.nullNode()
    }

    fun structToMap(source: Struct): Map<String, Any?> = structToMapDirect(source)

    internal fun structToMapViaObjectMapper(source: Struct): Map<String, Any?> {
        return try {
            val json = jsonPrinter.print(source)
            objectMapper.readValue(json, typeReference)
        } catch (e: InvalidProtocolBufferException) {
            throw RuntimeException(e)
        } catch (e: JsonProcessingException) {
            throw RuntimeException(e)
        }
    }

    internal fun structToMapDirect(source: Struct): Map<String, Any?> = buildMap {
        source.fieldsMap.forEach { (key, value) ->
            this[key] = valueToMapValue(value)
        }
    }

    private fun valueToMapValue(value: Value): Any? {
        return when (value.kindCase) {
            KindCase.NUMBER_VALUE -> value.numberValue
            KindCase.STRING_VALUE -> value.stringValue
            KindCase.BOOL_VALUE -> value.boolValue
            KindCase.STRUCT_VALUE -> structToMapDirect(value.structValue)
            KindCase.LIST_VALUE -> value.listValue.valuesList.map { valueToMapValue(it) }
            KindCase.KIND_NOT_SET, KindCase.NULL_VALUE -> null
            else -> null
        }
    }

    fun objectToStruct(source: Any?): Struct {
        if (source == null) {
            return Struct.getDefaultInstance()
        }
        //return objectToStructViaString(source)
        return objectToStructViaTree(source)
    }

    fun messageToMap(source: Any?): Map<String, Any> {
        if (source == null) {
            return mapOf()
        }
        val node = objectMapper.valueToTree<ObjectNode>(source)
        return objectMapper.convertValue(node, typeReference)
    }

    internal fun objectToStructViaTree(source: Any): Struct {
        val tree = objectMapper.valueToTree<ObjectNode>(source)
        return objectNodeToStruct(tree)
    }

    internal fun objectToStructViaString(source: Any): Struct {
        val struct = Struct.newBuilder()
        return try {
            val json: String = objectMapper.writeValueAsString(source)
            jsonParse.merge(json, struct)
            if (struct.fieldsCount == 0) {
                Struct.getDefaultInstance()
            } else {
                struct.build()
            }
        } catch (e: JsonProcessingException) {
            throw RuntimeException(e)
        } catch (e: InvalidProtocolBufferException) {
            throw RuntimeException(e)
        }
    }

    fun objectNodeToStruct(node: ObjectNode): Struct {
        val structBuilder = Struct.newBuilder()
        node.fields().forEach { (key, node) ->
            structBuilder.putFields(key, jsonNodeToValue(node))
        }
        return structBuilder.build()
    }

    private fun jsonNodeToValue(node: JsonNode): Value {
        return when (node) {
            is TextNode -> Values.of(node.textValue())
            is NullNode -> Values.ofNull()
            is BooleanNode -> if (node.booleanValue()) FromMessageConverter.VALUE_TRUE else FromMessageConverter.VALUE_FALSE
            is NumericNode -> Values.of(node.doubleValue())
            is ArrayNode -> Values.of(arrayNodeToValueList(node))
            is ObjectNode -> Values.of(objectNodeToStruct(node))
            else -> throw RuntimeException("unsupported node type: ${node.nodeType}")
        }
    }

    private fun arrayNodeToValueList(node: ArrayNode): Iterable<Value> {
        return node.elements().asSequence().map { jsonNodeToValue(it) }.asIterable()
    }

    companion object {
        private val typeReference: TypeReference<Map<String, Any>> = object : TypeReference<Map<String, Any>>() {}
    }
}
