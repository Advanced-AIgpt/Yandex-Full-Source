package ru.yandex.alice.social.sharing.apphost.handlers

import NAppHostHttp.Http
import com.google.protobuf.InvalidProtocolBufferException
import com.google.protobuf.Message
import com.google.protobuf.Parser
import com.google.protobuf.util.JsonFormat
import ru.yandex.web.apphost.api.request.RequestContext

fun <T: Message> parse(
    request: Http.THttpRequest,
    parser: Parser<T>,
    builder: Message.Builder,
): T {
    val contentType = request.headersList.getContentType()
    val body = request.content
    if (contentType == "application/json") {
        if (!body.isValidUtf8) {
            throw ParseException("Invalid utf8")
        }
        JsonFormat.parser()
            .ignoringUnknownFields()
            .merge(body.toStringUtf8(), builder)
        return builder.build() as T
    } else if (contentType == null || contentType == "application/x-protobuf") {
        try {
            return parser.parseFrom(body)
        } catch (e: InvalidProtocolBufferException) {
            throw ParseException("Failed to parse proto from request", e)
        }
    } else {
        throw ParseException("Invalid content type")
    }
}

fun <T: Message> parse(
    context: RequestContext,
    parser: Parser<T>,
    builder: Message.Builder,
): T {
    val request = context
        .getSingleRequestItem(PROTO_HTTP_REQUEST)
        .getProtobufData(Http.THttpRequest.getDefaultInstance())
    return parse(request, parser, builder)
}

class ParseException(
    message: String,
    reason: Exception?
): Exception(message, reason) {
    constructor(message: String): this(message, null)
}
