package ru.yandex.alice.kronstadt.scenarios.tvmain

import com.fasterxml.jackson.core.JsonParser
import com.fasterxml.jackson.databind.DeserializationContext
import com.fasterxml.jackson.databind.JsonDeserializer
import com.fasterxml.jackson.databind.JsonNode
import org.springframework.stereotype.Component
import java.io.IOException


@Component
class VhDocumentDeserializer: JsonDeserializer<BaseDocument>() {
    override fun deserialize(p: JsonParser?, ctxt: DeserializationContext?): BaseDocument {
        val node: JsonNode = p?.readValueAsTree()?: throw IOException("Cannot parse BaseDocument as tree: invalid JSON")

        return if (node.get("banned")?.asBoolean() == true) {
            BannedDocument(true)
        } else {
            if (node.path("ottParams").isMissingNode) {
                p.codec.treeToValue(node, VodEpisodeInfo::class.java)
            } else {
                p.codec.treeToValue(node, EpisodeInfo::class.java)
            }
        }
    }
}
