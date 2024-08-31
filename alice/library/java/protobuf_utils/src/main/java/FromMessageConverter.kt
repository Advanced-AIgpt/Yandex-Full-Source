package ru.yandex.alice.library.protobufutils

import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.node.ArrayNode
import com.fasterxml.jackson.databind.node.JsonNodeFactory
import com.fasterxml.jackson.databind.node.ObjectNode
import com.google.common.io.BaseEncoding
import com.google.protobuf.BoolValue
import com.google.protobuf.ByteString
import com.google.protobuf.BytesValue
import com.google.protobuf.Descriptors
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Descriptors.FieldDescriptor.Type
import com.google.protobuf.DoubleValue
import com.google.protobuf.Duration
import com.google.protobuf.DynamicMessage
import com.google.protobuf.FieldMask
import com.google.protobuf.FloatValue
import com.google.protobuf.Int32Value
import com.google.protobuf.Int64Value
import com.google.protobuf.InvalidProtocolBufferException
import com.google.protobuf.ListValue
import com.google.protobuf.Message
import com.google.protobuf.MessageOrBuilder
import com.google.protobuf.StringValue
import com.google.protobuf.Struct
import com.google.protobuf.Timestamp
import com.google.protobuf.TypeRegistry
import com.google.protobuf.UInt32Value
import com.google.protobuf.UInt64Value
import com.google.protobuf.Value
import com.google.protobuf.util.Durations
import com.google.protobuf.util.FieldMaskUtil
import com.google.protobuf.util.Timestamps
import com.google.protobuf.util.Values
import java.math.BigInteger
import java.util.TreeMap

data class FromMessageConverter(
    val registry: TypeRegistry = TypeRegistry.getEmptyTypeRegistry(),
    val alwaysOutputDefaultValueFields: Boolean = false,
    val includingDefaultValueFields: Set<Descriptors.FieldDescriptor> = setOf(),
    val preservingProtoFieldNames: Boolean = false,
    val printingEnumsAsInts: Boolean = false,
    val sortingMapKeys: Boolean = false,
) {
    private val toMapConverter = ToMapConverter()
    private val toObjectNodeConverter = ToObjectNodeConverter()
    private val toStructConverter = ToStructConverter()

    fun usingTypeRegistry(registry: TypeRegistry) = this.copy(registry = registry)
    fun alwaysOutputDefaultValueFields() = this.copy(alwaysOutputDefaultValueFields = true)
    fun includingDefaultValueFields(includingDefaultValueFields: Set<FieldDescriptor>) =
        this.copy(includingDefaultValueFields = includingDefaultValueFields)

    fun printingEnumsAsInts() = this.copy(printingEnumsAsInts = true)
    fun sortingMapKeys() = this.copy(sortingMapKeys = true)

    fun convertToMap(message: MessageOrBuilder): Map<String, Any?> = toMapConverter.convertToMap(message)
    fun convertToObjectNode(message: MessageOrBuilder): ObjectNode = toObjectNodeConverter.convertToObjectNode(message)
    fun convertToStruct(message: MessageOrBuilder): Struct = toStructConverter.convertToStruct(message)

    internal interface Node {
        fun wrap(): WrappedNode
    }

    internal interface ObjNode : Node {
        fun put(key: String, value: WrappedNode)
    }

    internal interface ArrNode : Node {
        fun add(value: WrappedNode)
    }

    internal interface WrappedNode {
        fun asObjNode(): ObjNode
    }

    @JvmInline
    private value class ToMapObjNodeImpl(val struct: MutableMap<String, Any?> = mutableMapOf()) : ObjNode {
        override fun put(key: String, value: WrappedNode) {
            struct[key] = (value as ToMapNodeImpl).value
        }

        override fun wrap(): WrappedNode = ToMapNodeImpl(struct)
    }

    @JvmInline
    private value class ToMapArrayNodeImpl(val items: MutableList<Any?>) : ArrNode {
        override fun add(value: WrappedNode) {
            items += (value as ToMapNodeImpl).value
        }

        override fun wrap(): WrappedNode = ToMapNodeImpl(items)
    }

    @JvmInline
    private value class ToMapNodeImpl(val value: Any?) : Node, WrappedNode {
        override fun wrap(): WrappedNode = this
        override fun asObjNode(): ObjNode = ToMapObjNodeImpl((value as Map<String, Any?>).toMutableMap())
    }

    inner class ToMapConverter() : AbstractMessageConverter() {

        fun convertToMap(message: MessageOrBuilder): Map<String, Any?> {
            return (convertToContainer(message) as ToMapObjNodeImpl).struct.toMap()
        }

        override fun newObj(size: Int): ObjNode = ToMapObjNodeImpl()

        override fun newArr(size: Int): ArrNode = ToMapArrayNodeImpl(items = ArrayList(size))

        override fun newNode(value: String): Node = ToMapNodeImpl(value)
        override fun newNode(value: Int): Node = ToMapNodeImpl(value.toDouble())
        override fun newNode(value: Long): Node = ToMapNodeImpl(value.toDouble())
        override fun newNode(value: Boolean): Node = ToMapNodeImpl(value)
        override fun newNode(value: Double): Node = ToMapNodeImpl(value)
        override fun newNullNode(): Node = ToMapNodeImpl(null)
    }

    @JvmInline
    private value class ToObjectNodeObjNodeImpl(val obj: ObjectNode) : ObjNode {
        override fun put(key: String, value: WrappedNode) {
            obj.set<ObjectNode>(key, (value as ToObjectNodeNodeImpl).value)
        }

        override fun wrap(): WrappedNode = ToObjectNodeNodeImpl(obj)
    }

    @JvmInline
    private value class ToObjectNodeArrayNodeImpl(val arr: ArrayNode) : ArrNode {
        override fun add(value: WrappedNode) {
            arr.add((value as ToObjectNodeNodeImpl).value)
        }

        override fun wrap(): WrappedNode = ToObjectNodeNodeImpl(arr)
    }

    @JvmInline
    private value class ToObjectNodeNodeImpl(val value: JsonNode) : Node, WrappedNode {
        override fun wrap(): WrappedNode = this
        override fun asObjNode(): ObjNode = ToObjectNodeObjNodeImpl(value as ObjectNode)
    }

    inner class ToObjectNodeConverter() : AbstractMessageConverter() {

        private val jsonNodeFactory = JsonNodeFactory.instance

        fun convertToObjectNode(message: MessageOrBuilder): ObjectNode =
            (convertToContainer(message) as ToObjectNodeObjNodeImpl).obj

        override fun newObj(size: Int): ObjNode = ToObjectNodeObjNodeImpl(jsonNodeFactory.objectNode())

        override fun newArr(size: Int): ArrNode = ToObjectNodeArrayNodeImpl(arr = jsonNodeFactory.arrayNode(size))

        override fun newNode(value: String): Node = ToObjectNodeNodeImpl(jsonNodeFactory.textNode(value))
        override fun newNode(value: Int): Node = ToObjectNodeNodeImpl(jsonNodeFactory.numberNode(value))
        override fun newNode(value: Long): Node = ToObjectNodeNodeImpl(jsonNodeFactory.numberNode(value))
        override fun newNode(value: Boolean): Node = ToObjectNodeNodeImpl(jsonNodeFactory.booleanNode(value))
        override fun newNode(value: Double): Node = ToObjectNodeNodeImpl(jsonNodeFactory.numberNode(value))
        override fun newNullNode(): Node = ToObjectNodeNodeImpl(jsonNodeFactory.nullNode())
    }

    @JvmInline
    private value class ToStructObjNodeImpl(val struct: Struct.Builder) : ObjNode {
        override fun put(key: String, value: WrappedNode) {
            struct.putFields(key, (value as ToStructNodeImpl).value)
        }

        override fun wrap(): WrappedNode = ToStructNodeImpl(Value.newBuilder().setStructValue(struct).build())
    }

    @JvmInline
    private value class ToStructArrayNodeImpl(val items: ListValue.Builder = ListValue.newBuilder()) : ArrNode {
        override fun add(value: WrappedNode) {
            items.addValues((value as ToStructNodeImpl).value)
        }

        override fun wrap(): WrappedNode = ToStructNodeImpl(Value.newBuilder().setListValue(items).build())
    }

    @JvmInline
    private value class ToStructNodeImpl(val value: Value) : Node, WrappedNode {
        override fun wrap(): WrappedNode = this
        override fun asObjNode(): ObjNode = ToStructObjNodeImpl(value.structValue.toBuilder())
    }

    inner class ToStructConverter() : AbstractMessageConverter() {

        fun convertToStruct(message: MessageOrBuilder): Struct {
            if (message is Struct) {
                return message
            } else if (message is Struct.Builder) {
                return message.build()
            }
            return (convertToContainer(message) as ToStructObjNodeImpl).struct.build()
        }

        override fun newObj(size: Int): ObjNode = ToStructObjNodeImpl(Struct.newBuilder())

        override fun newArr(size: Int): ArrNode = ToStructArrayNodeImpl()

        override fun newNode(value: String): Node = ToStructNodeImpl(Values.of(value))
        override fun newNode(value: Int): Node = ToStructNodeImpl(Values.of(value.toDouble()))
        override fun newNode(value: Long): Node = ToStructNodeImpl(Values.of(value.toDouble()))
        override fun newNode(value: Boolean): Node = ToStructNodeImpl(if (value) VALUE_TRUE else VALUE_FALSE)
        override fun newNode(value: Double): Node = ToStructNodeImpl(Values.of(value))
        override fun newNullNode(): Node = ToStructNodeImpl(Values.ofNull())

        override fun printValue(message: MessageOrBuilder): WrappedNode = ToStructNodeImpl(message as Value)
    }

    abstract inner class AbstractMessageConverter() {

        private val wellKnownTypePrinters: Map<String, (AbstractMessageConverter, MessageOrBuilder) -> WrappedNode> =
            buildMap {
                // Special-case Any.
                this[com.google.protobuf.Any.getDescriptor().fullName] =
                    { printer, message -> printer.printAny(message) }

                // Special-case wrapper types.
                val wrappersPrinter: (AbstractMessageConverter, MessageOrBuilder) -> WrappedNode =
                    { printer, message -> printer.printWrapper(message) }
                this[BoolValue.getDescriptor().fullName] = wrappersPrinter
                this[Int32Value.getDescriptor().fullName] = wrappersPrinter
                this[UInt32Value.getDescriptor().fullName] = wrappersPrinter
                this[Int64Value.getDescriptor().fullName] = wrappersPrinter
                this[UInt64Value.getDescriptor().fullName] = wrappersPrinter
                this[StringValue.getDescriptor().fullName] = wrappersPrinter
                this[BytesValue.getDescriptor().fullName] = wrappersPrinter
                this[FloatValue.getDescriptor().fullName] = wrappersPrinter
                this[DoubleValue.getDescriptor().fullName] = wrappersPrinter

                // Special-case Timestamp.
                this[Timestamp.getDescriptor().fullName] = { printer, message -> printer.printTimestamp(message) }

                // Special-case Duration.
                this[Duration.getDescriptor().fullName] = { printer, message -> printer.printDuration(message) }

                // Special-case FieldMask.
                this[FieldMask.getDescriptor().fullName] = { printer, message -> printer.printFieldMask(message) }

                // Special-case Struct.
                this[Struct.getDescriptor().fullName] = { printer, message -> printer.printStruct(message) }

                // Special-case Value.
                this[Value.getDescriptor().fullName] = { printer, message -> printer.printValue(message) }

                // Special-case ListValue.
                this[ListValue.getDescriptor().fullName] = { printer, message -> printer.printListValue(message) }
            }

        internal abstract fun newObj(size: Int): ObjNode
        internal abstract fun newArr(size: Int): ArrNode

        internal abstract fun newNode(value: String): Node
        internal abstract fun newNode(value: Int): Node
        internal abstract fun newNode(value: Long): Node
        internal abstract fun newNode(value: Boolean): Node
        internal abstract fun newNode(value: Double): Node
        internal abstract fun newNullNode(): Node

        internal fun convertToContainer(message: MessageOrBuilder): ObjNode {
            return wellKnownTypePrinters[message.descriptorForType.fullName]
                ?.let { specialPrinter -> specialPrinter(this, message).asObjNode() }
                ?: print(message, null)
        }

        private fun printAny(message: MessageOrBuilder): WrappedNode {
            if (com.google.protobuf.Any.getDefaultInstance() == message) {
                return newObj(0).wrap()
            }
            val descriptor = message.descriptorForType
            val typeUrlField = descriptor.findFieldByName("type_url")
            val valueField = descriptor.findFieldByName("value")
            // Validates type of the message. Note that we can't just cast the message
            // to com.google.protobuf.Any because it might be a DynamicMessage.
            if (typeUrlField == null || valueField == null || typeUrlField.type != Type.STRING || valueField.type != Type.BYTES) {
                throw InvalidProtocolBufferException("Invalid Any type.")
            }
            val typeUrl = message.getField(typeUrlField) as String
            val type: Descriptors.Descriptor = registry.getDescriptorForTypeUrl(typeUrl)
                ?: throw InvalidProtocolBufferException("Cannot find type for url: $typeUrl")

            val content = message.getField(valueField) as ByteString
            val contentMessage: Message = DynamicMessage.getDefaultInstance(type).parserForType.parseFrom(content)
            val printer = wellKnownTypePrinters[getTypeName(typeUrl)]

            return if (printer != null) {
                // If the type is one of the well-known types, we use a special
                // formatting.
                newObj(2).apply {
                    put("@type", newNode(typeUrl).wrap())
                    put("value", printer(this@AbstractMessageConverter, contentMessage))
                }
            } else {
                // Print the content message instead (with a "@type" field added).
                print(contentMessage, typeUrl)
            }.wrap()
        }

        private fun printWrapper(message: MessageOrBuilder): WrappedNode {
            val descriptor = message.descriptorForType
            val valueField = descriptor.findFieldByName("value")
                ?: throw InvalidProtocolBufferException("Invalid Wrapper type.")
            // When formatting wrapper types, we just print its value field instead of
            // the whole message.
            return printSingleFieldValue(valueField, message.getField(valueField))
        }

        private fun toByteString(message: MessageOrBuilder): ByteString {
            return if (message is Message) {
                message.toByteString()
            } else {
                (message as Message.Builder).build().toByteString()
            }
        }

        /** Prints google.protobuf.Timestamp  */
        private fun printTimestamp(message: MessageOrBuilder): WrappedNode {
            val value = Timestamp.parseFrom(toByteString(message))
            return newNode(Timestamps.toString(value)).wrap()
        }

        /** Prints google.protobuf.Duration */
        private fun printDuration(message: MessageOrBuilder): WrappedNode {
            val value = Duration.parseFrom(toByteString(message))
            return newNode(Durations.toString(value)).wrap()
        }

        /** Prints google.protobuf.FieldMask  */
        private fun printFieldMask(message: MessageOrBuilder): WrappedNode {
            val value = FieldMask.parseFrom(toByteString(message))
            return newNode(FieldMaskUtil.toJsonString(value)).wrap()
        }

        /** Prints google.protobuf.Struct  */
        private fun printStruct(message: MessageOrBuilder): WrappedNode {
            val descriptor = message.descriptorForType
            val field = descriptor.findFieldByName("fields")
                ?: throw InvalidProtocolBufferException("Invalid Struct type.")

            // Struct is formatted as a map object.
            return printMapFieldValue(field, message.getField(field))
        }

        /** Prints google.protobuf.Value  */
        internal open fun printValue(message: MessageOrBuilder): WrappedNode {
            val v = message as Value
            return when (v.kindCase) {
                Value.KindCase.NULL_VALUE -> newNullNode().wrap()
                Value.KindCase.NUMBER_VALUE -> newNode(v.numberValue).wrap()
                Value.KindCase.STRING_VALUE -> newNode(v.stringValue).wrap()
                Value.KindCase.BOOL_VALUE -> newNode(v.boolValue).wrap()
                Value.KindCase.STRUCT_VALUE -> printStruct(v.structValue)
                Value.KindCase.LIST_VALUE -> printListValue(v.listValue)
                Value.KindCase.KIND_NOT_SET -> newNullNode().wrap()
            }
        }

        /** Prints google.protobuf.ListValue  */
        private fun printListValue(message: MessageOrBuilder): WrappedNode {
            val descriptor = message.descriptorForType
            val field = descriptor.findFieldByName("values")
                ?: throw InvalidProtocolBufferException("Invalid ListValue type.")
            return printRepeatedFieldValue(field, message.getField(field))
        }

        /** Prints a regular message with an optional type URL.  */
        private fun print(message: MessageOrBuilder, typeUrl: String?): ObjNode {

            val fieldsToPrint: MutableMap<FieldDescriptor, Any>
            if (alwaysOutputDefaultValueFields || includingDefaultValueFields.isNotEmpty()) {
                fieldsToPrint = TreeMap(message.allFields)
                for (field in message.descriptorForType.fields) {
                    if (field.isOptional) {
                        if (field.javaType == FieldDescriptor.JavaType.MESSAGE
                            && !message.hasField(field)
                        ) {
                            // Always skip empty optional message fields. If not we will recurse indefinitely if
                            // a message has itself as a sub-field.
                            continue
                        }
                        val oneof = field.containingOneof
                        if (oneof != null && !message.hasField(field)) {
                            // Skip all oneof fields except the one that is actually set
                            continue
                        }
                    }
                    if (!fieldsToPrint.containsKey(field)
                        && (alwaysOutputDefaultValueFields || includingDefaultValueFields.contains(field))
                    ) {
                        fieldsToPrint[field] = message.getField(field)
                    }
                }
            } else {
                fieldsToPrint = message.allFields
            }

            val data = newObj(fieldsToPrint.size + if (typeUrl != null) 1 else 0).apply {
                if (typeUrl != null) {
                    put("@type", newNode(typeUrl).wrap())
                }
                for ((field, value) in fieldsToPrint) {
                    //printField(strBuilder, key, value)
                    val name = if (preservingProtoFieldNames) field.name else field.jsonName
                    put(name, printField(field, value))
                }
            }

            return data
        }

        // put Field to struct
        private fun printField(field: FieldDescriptor, value: Any): WrappedNode {

            return if (field.isMapField) {
                printMapFieldValue(field, value)
            } else if (field.isRepeated) {
                printRepeatedFieldValue(field, value)
            } else {
                printSingleFieldValue(field, value)
            }
        }

        @Suppress("UNCHECKED_CAST")
        private fun printRepeatedFieldValue(
            field: FieldDescriptor,
            value: Any
        ): WrappedNode {
            val list = (value as List<Any>)
            val arr = newArr(list.size)
            list.forEach {
                arr.add(printSingleFieldValue(field, it))
            }

            return arr.wrap()
        }

        @Suppress("UNCHECKED_CAST")
        private fun printMapFieldValue(field: FieldDescriptor, value: Any): WrappedNode {
            val type = field.messageType
            val keyField = type.findFieldByName("key")
            val valueField = type.findFieldByName("value")
            if (keyField == null || valueField == null) {
                throw InvalidProtocolBufferException("Invalid map field.")
            }
            var elements:  // Object guaranteed to be a List for a map field.
                Collection<Any> = value as List<Any>
            if (sortingMapKeys && !elements.isEmpty()) {
                var cmp: Comparator<Any>? = null
                if (keyField.type == Type.STRING) {
                    cmp = Comparator { o1, o2 ->
                        val s1 = ByteString.copyFromUtf8(o1 as String)
                        val s2 = ByteString.copyFromUtf8(o2 as String)
                        ByteString.unsignedLexicographicalComparator().compare(s1, s2)
                    }
                }
                val tm = TreeMap<Any, Any>(cmp)
                for (element in elements) {
                    val entry = element as Message
                    val entryKey = entry.getField(keyField)
                    tm[entryKey] = element
                }
                elements = tm.values
            }

            return newObj(elements.size).apply {
                for (element in elements) {
                    val entry = element as Message
                    val entryKey = entry.getField(keyField)
                    val entryValue = entry.getField(valueField)

                    // Key fields are always double-quoted.
                    val keyValue = printMapKey(keyField, entryKey)
                    val valVale = printSingleFieldValue(valueField, entryValue)
                    //strBuilder.putFields(keyValue, valVale)
                    put(keyValue, valVale)
                }
            }.wrap()
        }

        @Suppress("WHEN_ENUM_CAN_BE_NULL_IN_JAVA")
        private fun printSingleFieldValue(
            field: FieldDescriptor, value: Any
        ): WrappedNode {
            return when (field.type) {
                Type.INT32, Type.SINT32, Type.SFIXED32 -> newNode(value as Int)
                Type.INT64, Type.SINT64, Type.SFIXED64 -> newNode(value as Long)
                Type.BOOL -> newNode(value as Boolean)
                Type.FLOAT -> {
                    val floatValue = value as Float
                    if (floatValue.isNaN()) {
                        newNode("NaN")
                    } else if (floatValue.isInfinite()) {
                        if (floatValue < 0) {
                            newNode("-Infinity")
                        } else {
                            newNode("Infinity")
                        }
                    } else {
                        newNode(floatValue.toDouble())
                    }
                }
                Type.DOUBLE -> {
                    val doubleValue = value as Double
                    if (doubleValue.isNaN()) {
                        newNode("NaN")
                    } else if (doubleValue.isInfinite()) {
                        if (doubleValue < 0) {
                            newNode("-Infinity")
                        } else {
                            newNode("Infinity")
                        }
                    } else {
                        newNode(doubleValue)
                    }
                }
                Type.UINT32, Type.FIXED32 -> newNode(unsignedToDouble(value as Int))
                Type.UINT64, Type.FIXED64 -> newNode(unsignedToDouble(value as Long))
                Type.STRING -> newNode(value as String)
                Type.BYTES -> newNode((value as ByteString).toBase64())
                Type.ENUM -> {
                    // Special-case google.protobuf.NullValue (it's an Enum).
                    if (field.enumType.fullName == "google.protobuf.NullValue") {
                        // No matter what value it contains, we always print it as "null".
                        newNullNode()
                    } else {
                        if (printingEnumsAsInts || (value as Descriptors.EnumValueDescriptor).index == -1) {
                            newNode((value as Descriptors.EnumValueDescriptor).number.toString())
                        } else {
                            newNode(value.name)
                        }
                    }
                }
                Type.MESSAGE, Type.GROUP -> convertToContainer(value as Message)
            }.wrap()
        }

        private fun printMapKey(
            field: FieldDescriptor, value: Any
        ): String {
            return when (field.type) {
                Type.INT32, Type.SINT32, Type.SFIXED32 -> value.toString()

                Type.INT64, Type.SINT64, Type.SFIXED64 -> value.toString()
                Type.BOOL -> if ((value as Boolean)) "true" else "false"
                Type.FLOAT -> {
                    val floatValue = value as Float
                    if (floatValue.isNaN()) {
                        "NaN"
                    } else if (floatValue.isInfinite()) {
                        if (floatValue < 0) {
                            "-Infinity"
                        } else {
                            "Infinity"
                        }
                    } else {
                        floatValue.toString()
                    }
                }
                Type.DOUBLE -> {
                    val doubleValue = value as Double
                    if (doubleValue.isNaN()) {
                        "NaN"
                    } else if (doubleValue.isInfinite()) {
                        if (doubleValue < 0) {
                            "-Infinity"
                        } else {
                            "Infinity"
                        }
                    } else {
                        doubleValue.toString()
                    }
                }
                Type.UINT32, Type.FIXED32 -> value.toString()
                Type.UINT64, Type.FIXED64 -> value.toString()
                Type.STRING -> value as String
                Type.BYTES -> (value as ByteString).toBase64()
                Type.ENUM -> {
                    // Special-case google.protobuf.NullValue (it's an Enum).
                    if (field.enumType.fullName == "google.protobuf.NullValue") {
                        // No matter what value it contains, we always print it as "null".
                        "null"
                    } else {
                        if (printingEnumsAsInts || (value as Descriptors.EnumValueDescriptor).index == -1) {
                            (value as Descriptors.EnumValueDescriptor).number.toString()
                        } else {
                            value.name
                        }
                    }
                }
                else -> throw InvalidProtocolBufferException("Invalid map key type: ${field.type}")
            }
        }

        private fun ByteString.toBase64(): String = BaseEncoding.base64().encode(this.toByteArray())

        /** Convert an unsigned 32-bit integer to a double. */
        private fun unsignedToDouble(value: Int): Double {
            return if (value >= 0) {
                value.toDouble()
            } else {
                (value.toLong() and 0x00000000FFFFFFFFL).toDouble()
            }
        }

        /** Convert an unsigned 64-bit integer to a double.  */
        private fun unsignedToDouble(value: Long): Double {
            return if (value >= 0) {
                value.toDouble()
            } else {
                // Pull off the most-significant bit so that BigInteger doesn't think
                // the number is negative, then set it again using setBit().
                BigInteger.valueOf(value and Long.MAX_VALUE).setBit(Long.SIZE_BITS - 1).toDouble()
            }
        }
    }

    companion object {

        @JvmField
        val DEFAULT_CONVERTER = FromMessageConverter()

        internal val VALUE_TRUE = Values.of(true)
        internal val VALUE_FALSE = Values.of(false)

        private fun getTypeName(typeUrl: String): String {
            val parts = typeUrl.split("/").toTypedArray()
            if (parts.size == 1) {
                throw InvalidProtocolBufferException("Invalid type url found: $typeUrl")
            }
            return parts.last()
        }
    }
}
