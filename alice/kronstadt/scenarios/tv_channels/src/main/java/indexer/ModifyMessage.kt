package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.annotation.JsonAnyGetter
import com.fasterxml.jackson.annotation.JsonIgnore
import com.fasterxml.jackson.annotation.JsonProperty

class ModifyMessage(
    uri: String,
    override val prefix: Int = 1,
) : SaasMessage {

    @JsonIgnore
    private val doc: Document = Document(uri)
    override val action: SaasAction = SaasAction.modify

    data class Options(
        @JsonProperty("mime_type")
        val mimeType: String = "text/plain",
        @JsonProperty("charset")
        val charset: String = "utf8",
        @JsonProperty("language")
        val language: String = "ru",
    ) {
        companion object {
            val DEFAULT: Options = Options()
        }
    }

    @JsonProperty("docs")
    fun docs() = listOf(doc)

    fun addZone(name: String, content: Any): ModifyMessage {
        if (name in doc.zones || name in doc.attributes) {
            throw IllegalArgumentException("zone $name already presented in the document")
        }
        doc.zones[name] = TypedItem(content, TypedItemType.ZONE)
        return this
    }

    fun addAttr(name: String, value: Any, vararg types: TypedItemType): ModifyMessage {
        if (name in doc.zones) {
            throw IllegalArgumentException("Attribute $name is zone, not an attribute. Use #addZone method for zones")
        }
        val typesSet = setOf(*types)
        if (typesSet.contains(TypedItemType.ZONE)) {
            throw IllegalArgumentException("incorrect syntax on attribute ${name}: using #z with other item types (#lgp). User #addZone method for zones")
        }

        doc.attributes.computeIfAbsent(name) { mutableListOf() }
            .add(TypedItem(value, typesSet))
        return this
    }

    class Document(
        val url: String,
        val options: Options = Options.DEFAULT,
    ) {
        @JsonIgnore
        val zones: MutableMap<String, TypedItem> = mutableMapOf()

        @JsonIgnore
        val attributes: MutableMap<String, MutableList<TypedItem>> = mutableMapOf()

        @JsonAnyGetter
        private fun getAll(): Map<String, Any> = buildMap(zones.size + attributes.size) {
            this.putAll(zones)
            this.putAll(attributes)
        }
    }
}
