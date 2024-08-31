package ru.yandex.alice.divkit

import com.fasterxml.jackson.annotation.JsonValue
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Message
import ru.yandex.alice.library.protobufutils.FromMessageConverter
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.scenarios.directive.TDirective
import ru.yandex.alice.protos.extensions.ExtensionsProto
import java.net.URI
import java.net.URLEncoder
import java.nio.charset.StandardCharsets

internal val jsonMapper: ObjectMapper = jacksonObjectMapper()

sealed interface Directive {
    val name: String
    val type: DirectiveType
    val payload: Any?
}

enum class DirectiveType(@JsonValue val code: String) {
    SERVER_ACTION("server_action"),
    CLIENT_ACTION("client_action"),
}

data class ServerAction(
    override val name: String,
    override val payload: Any?,
) : Directive {
    override val type = DirectiveType.SERVER_ACTION
}

data class ClientAction(
    override val name: String,
    override val payload: Any?,
) : Directive {
    override val type = DirectiveType.CLIENT_ACTION
}

fun FrameProto.TSemanticFrameRequestData.toServerAction() = ServerAction(
    name = "@@mm_semantic_frame",
    payload = FromMessageConverter.DEFAULT_CONVERTER.convertToObjectNode(this),
)

private val directivesData: Map<TDirective.DirectiveCase, DirectiveMeta> =
    buildMap(TDirective.DirectiveCase.values().size - 1) {
        val descriptor = TDirective.getDescriptor()
        TDirective.DirectiveCase.values()
            .filter { it != TDirective.DirectiveCase.DIRECTIVE_NOT_SET }
            .forEach {
                val field = descriptor.findFieldByNumber(it.number)
                this[it] = DirectiveMeta(
                    field = field,
                    speechKitName = field.messageType.options.getExtension(ExtensionsProto.speechKitName),
                    nameField = field.messageType.findFieldByName("name"),
                )
            }
    }

private class DirectiveMeta(
    val field: FieldDescriptor,
    val speechKitName: String,
    val nameField: FieldDescriptor?,
)

fun TDirective.toClientAction(): ClientAction? {
    val directiveMeta = directivesData[this.directiveCase]
    if (directiveMeta != null) {
        val directive = this.getField(directiveMeta.field) as Message
        val name: String? = directiveMeta.nameField?.let { directive.getField(it) as? String }

        return ClientAction(
            name = name ?: directiveMeta.speechKitName,
            payload = FromMessageConverter.DEFAULT_CONVERTER.convertToObjectNode(directive)
        )
    } else {
        return null
    }
}

internal fun createDialogActionUri(
    dialogId: String? = null,
    directives: List<Directive>? = null,
): URI {
    val params = mutableMapOf<String, String>()
    dialogId?.let {
        params["dialog_id"] = URLEncoder.encode(dialogId, StandardCharsets.UTF_8)
    }
    directives?.takeIf { it.isNotEmpty() }?.let {
        params["directives"] = URLEncoder.encode(jsonMapper.writeValueAsString(it), StandardCharsets.UTF_8)
    }
    return URI.create("dialog://?" + params.entries.joinToString(separator = "&") { (key, value) -> "$key=$value" })
}
