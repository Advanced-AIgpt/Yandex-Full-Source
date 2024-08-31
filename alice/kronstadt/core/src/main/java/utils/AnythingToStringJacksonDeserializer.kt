package ru.yandex.alice.kronstadt.core.utils

import com.fasterxml.jackson.core.JsonParser
import com.fasterxml.jackson.core.TreeNode
import com.fasterxml.jackson.databind.DeserializationContext
import com.fasterxml.jackson.databind.JsonDeserializer
import java.io.IOException

class AnythingToStringJacksonDeserializer : JsonDeserializer<String>() {

    @Throws(IOException::class)
    override fun deserialize(jp: JsonParser, ctxt: DeserializationContext): String {
        val tree = jp.codec.readTree<TreeNode>(jp)
        return tree.toString()
    }
}
