package ru.yandex.alice.kronstadt.core.convert

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import com.google.protobuf.Descriptors
import com.google.protobuf.Message
import com.google.protobuf.Struct
import com.google.protobuf.TypeRegistry
import org.springframework.stereotype.Component
import ru.yandex.alice.library.protobufutils.FromMessageConverter
import ru.yandex.alice.library.protobufutils.JacksonToProtoConverter

@Component
open class ProtoUtil(
    objectMapper: ObjectMapper,
    private val registry: TypeRegistry = TypeRegistry.getEmptyTypeRegistry()
) {
    private val jacksonToProtoConverter: JacksonToProtoConverter = JacksonToProtoConverter(objectMapper)
    private val defaultMessageToStructConverter = createFromMessageConverter()

    fun protoToMap(proto: Message): Map<String, Any?> {
        val struct = defaultMessageToStructConverter.convertToStruct(proto)
        return jacksonToProtoConverter.structToMap(struct)
    }

    fun objectToStruct(source: Any?): Struct = jacksonToProtoConverter.objectToStruct(source)
    fun objectNodeToStruct(source: ObjectNode): Struct = jacksonToProtoConverter.objectNodeToStruct(source)

    fun messageToStruct(source: Message): Struct = defaultMessageToStructConverter.convertToStruct(source)

    fun <T> structToObject(source: Struct, clazz: Class<T>): T = jacksonToProtoConverter.structToObject(source, clazz)
    inline fun <reified T> structToObject(source: Struct): T = structToObject(source, T::class.java)

    fun structToMap(source: Struct): Map<String, Any?> = jacksonToProtoConverter.structToMap(source)

    fun jsonStringToStruct(source: String?): Struct = jacksonToProtoConverter.jsonStringToStruct(source)

    fun messageToMap(source: Message): Map<String, Any?> = defaultMessageToStructConverter.convertToMap(source)
    fun messageToObjectNode(source: Message): ObjectNode = defaultMessageToStructConverter.convertToObjectNode(source)

    fun createFromMessageConverter(
        registry: TypeRegistry = this.registry,
        alwaysOutputDefaultValueFields: Boolean = false,
        includingDefaultValueFields: Set<Descriptors.FieldDescriptor> = setOf(),
        preservingProtoFieldNames: Boolean = false,
        printingEnumsAsInts: Boolean = false,
        sortingMapKeys: Boolean = false,
    ): FromMessageConverter = FromMessageConverter(
        registry = registry,
        alwaysOutputDefaultValueFields = alwaysOutputDefaultValueFields,
        includingDefaultValueFields = includingDefaultValueFields,
        preservingProtoFieldNames = preservingProtoFieldNames,
        printingEnumsAsInts = printingEnumsAsInts,
        sortingMapKeys = sortingMapKeys
    )
}
