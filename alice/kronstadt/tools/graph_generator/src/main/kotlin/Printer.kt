package ru.yandex.alice.kronstadt.generator

import NAppHostPbConfig.NNora.Graph
import NAppHostPbConfig.NNora.Monitoring
import com.fasterxml.jackson.core.JsonGenerator
import com.fasterxml.jackson.core.util.DefaultIndenter
import com.fasterxml.jackson.core.util.DefaultPrettyPrinter
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.ObjectWriter
import com.fasterxml.jackson.databind.SerializerProvider
import com.fasterxml.jackson.databind.node.ArrayNode
import com.fasterxml.jackson.databind.node.BaseJsonNode
import com.fasterxml.jackson.databind.node.JsonNodeFactory
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.util.JsonFormat
import ru.yandex.alice.kronstadt.tools.generator.TGraphWrapper
import java.io.File
import java.util.TreeMap

internal class Printer {
    internal class SortingNodeFactory : JsonNodeFactory() {
        override fun objectNode(): ObjectNode {
            return ObjectNode(this, TreeMap())
        }

        override fun arrayNode(): ArrayNode {
            return SortedArrayNode(this)
        }
    }

    private class SortedArrayNode(nf: JsonNodeFactory) : ArrayNode(nf) {
        override fun serialize(f: JsonGenerator, provider: SerializerProvider) {
            val children: MutableList<JsonNode> = this.elements().asSequence().toMutableList()
            this.elements().iterator()
            if (children.all { it.isTextual }) {
                children.sortBy { c -> c.textValue() }
            }

            val size: Int = children.size
            f.writeStartArray(this, size)
            children.forEach { (it as BaseJsonNode).serialize(f, provider) }
            f.writeEndArray()
        }
    }

    private val objectMapper: ObjectMapper = jacksonObjectMapper().copy()
        // sort keys in objects
        .setNodeFactory(SortingNodeFactory())

    private val prettyPrinter: MyPrinter = MyPrinter()

    private class MyPrinter() : DefaultPrettyPrinter("    ") {
        init {
            indentArraysWith(DefaultIndenter.SYSTEM_LINEFEED_INSTANCE.withIndent("    "))
            indentObjectsWith(DefaultIndenter().withIndent("    "))
        }

        override fun writeObjectFieldValueSeparator(g: JsonGenerator) {
            g.writeRaw(_separators.objectFieldValueSeparator + " ")
        }

        override fun createInstance(): MyPrinter = MyPrinter()
    }

    private val writer: ObjectWriter = objectMapper.writer(prettyPrinter)

    fun printGraph(graphFile: File, graph: TGraphWrapper) {
        graphFile.writeText(generateJson(graph))
    }

    private fun generateJson(graph: TGraphWrapper): String {
        val rawJson = JsonFormat.printer()
            .preservingProtoFieldNames()
            .includingDefaultValueFields(
                setOf(
                    Graph.TNode.getDescriptor().findFieldByNumber(Graph.TNode.NODE_TYPE_FIELD_NUMBER),
                    Monitoring.TAlert.getDescriptor().findFieldByNumber(Monitoring.TAlert.TYPE_FIELD_NUMBER)
                )
            )
            .print(graph)
        val tree = objectMapper.readTree(rawJson)
        return writer.writeValueAsString(tree) + "\n"
    }
}
