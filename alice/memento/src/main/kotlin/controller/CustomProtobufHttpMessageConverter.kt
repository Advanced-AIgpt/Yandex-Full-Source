package ru.yandex.alice.memento.controller

import com.google.protobuf.CodedOutputStream
import com.google.protobuf.ExtensionRegistry
import com.google.protobuf.Message
import com.google.protobuf.TextFormat
import com.google.protobuf.util.JsonFormat
import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpInputMessage
import org.springframework.http.HttpOutputMessage
import org.springframework.http.MediaType
import org.springframework.http.converter.AbstractHttpMessageConverter
import org.springframework.http.converter.HttpMessageConversionException
import org.springframework.http.converter.HttpMessageNotReadableException
import org.springframework.http.converter.HttpMessageNotWritableException
import org.springframework.util.ConcurrentReferenceHashMap
import java.io.IOException
import java.io.InputStream
import java.io.InputStreamReader
import java.io.OutputStream
import java.io.OutputStreamWriter
import java.lang.reflect.Method
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets

internal class CustomProtobufHttpMessageConverter
@JvmOverloads constructor(
    parser: JsonFormat.Parser?,
    printer: JsonFormat.Printer?,
    extensionRegistry: ExtensionRegistry? = null
) : AbstractHttpMessageConverter<Message>() {

    private val extensionRegistry = extensionRegistry ?: ExtensionRegistry.newInstance()
    private val protobufFormatSupport = ProtobufJavaUtilSupport(
        parser ?: JsonFormat.parser(),
        printer ?: JsonFormat.printer()
    )

    init {
        supportedMediaTypes = listOf(*protobufFormatSupport.supportedMediaTypes())
    }

    override fun supports(clazz: Class<*>): Boolean {
        return Message::class.java.isAssignableFrom(clazz)
    }

    override fun getDefaultContentType(message: Message): MediaType {
        return PROTOBUF
    }

    @Throws(IOException::class, HttpMessageNotReadableException::class)
    override fun readInternal(clazz: Class<out Message>, inputMessage: HttpInputMessage): Message {
        val builder: Message.Builder
        try {
            val contentType = inputMessage.headers.contentType ?: PROTOBUF
            val charset = contentType.charset ?: DEFAULT_CHARSET

            builder = getMessageBuilder(clazz)
            if (PROTOBUF.isCompatibleWith(contentType) || X_PROTOBUF.isCompatibleWith(contentType)) {
                builder.mergeFrom(inputMessage.body, extensionRegistry)
            } else if (MediaType.TEXT_PLAIN.isCompatibleWith(contentType)) {
                val reader = InputStreamReader(inputMessage.body, charset)
                TextFormat.merge(reader, extensionRegistry, builder)
            } else if (protobufFormatSupport != null) {
                protobufFormatSupport.merge(
                    inputMessage.body, charset, contentType, extensionRegistry, builder
                )
            }
        } catch (e: Exception) {
            Companion.logger.error("Read protobuf message exception", e)
            throw e
        }
        return builder.build()
    }

    /**
     * Create a new `Message.Builder` instance for the given class.
     *
     * This method uses a ConcurrentReferenceHashMap for caching method lookups.
     */
    private fun getMessageBuilder(clazz: Class<out Message>): Message.Builder {
        return try {
            var method = METHOD_CACHE[clazz]
            if (method == null) {
                method = clazz.getMethod("newBuilder")
                METHOD_CACHE[clazz] = method
            }
            method!!.invoke(clazz) as Message.Builder
        } catch (ex: Exception) {
            throw HttpMessageConversionException(
                "Invalid Protobuf Message type: no invocable newBuilder() method on $clazz", ex
            )
        }
    }

    override fun canWrite(mediaType: MediaType?): Boolean {
        return super.canWrite(mediaType) ||
            protobufFormatSupport != null && protobufFormatSupport.supportsWriteOnly(mediaType)
    }

    @Throws(IOException::class, HttpMessageNotWritableException::class)
    override fun writeInternal(message: Message, outputMessage: HttpOutputMessage) {
        val contentType = outputMessage.headers.contentType ?: getDefaultContentType(message)
        val charset = contentType.charset ?: DEFAULT_CHARSET

        if (PROTOBUF.isCompatibleWith(contentType) || X_PROTOBUF.isCompatibleWith(contentType)) {
            setProtoHeader(outputMessage, message)
            val codedOutputStream = CodedOutputStream.newInstance(outputMessage.body)
            message.writeTo(codedOutputStream)
            codedOutputStream.flush()
        } else if (MediaType.TEXT_PLAIN.isCompatibleWith(contentType)) {
            val outputStreamWriter = OutputStreamWriter(outputMessage.body, charset)
            TextFormat.printer().print(message, outputStreamWriter)
            outputStreamWriter.flush()
            outputMessage.body.flush()
        } else if (protobufFormatSupport != null) {
            protobufFormatSupport.print(message, outputMessage.body, contentType, charset)
            outputMessage.body.flush()
        }
    }

    /**
     * Set the "X-Protobuf-*" HTTP headers when responding with a message of
     * content type "application/x-protobuf"
     *
     * **Note:** `outputMessage.getBody()` should not have been called
     * before because it writes HTTP headers (making them read only).
     */
    private fun setProtoHeader(response: HttpOutputMessage, message: Message) {
        response.headers[X_PROTOBUF_SCHEMA_HEADER] = message.descriptorForType.file.name
        response.headers[X_PROTOBUF_MESSAGE_HEADER] = message.descriptorForType.fullName
    }

    /**
     * Protobuf format support.
     */
    internal interface ProtobufFormatSupport {
        fun supportedMediaTypes(): Array<MediaType>
        fun supportsWriteOnly(mediaType: MediaType?): Boolean

        @Throws(IOException::class, HttpMessageConversionException::class)
        fun merge(
            input: InputStream,
            charset: Charset,
            contentType: MediaType,
            extensionRegistry: ExtensionRegistry,
            builder: Message.Builder
        )

        @Throws(IOException::class, HttpMessageConversionException::class)
        fun print(message: Message, output: OutputStream, contentType: MediaType, charset: Charset)
    }

    /**
     * [ProtobufFormatSupport] implementation used when
     * `com.google.protobuf.util.JsonFormat` is available.
     */
    private class ProtobufJavaUtilSupport(parser: JsonFormat.Parser?, printer: JsonFormat.Printer?) : ProtobufFormatSupport {
        private val parser: JsonFormat.Parser = parser ?: JsonFormat.parser()
        private val printer: JsonFormat.Printer = printer ?: JsonFormat.printer()
        override fun supportedMediaTypes(): Array<MediaType> {
            return arrayOf(PROTOBUF, X_PROTOBUF, MediaType.TEXT_PLAIN, MediaType.APPLICATION_JSON)
        }

        override fun supportsWriteOnly(mediaType: MediaType?): Boolean {
            return false
        }

        @Throws(IOException::class, HttpMessageConversionException::class)
        override fun merge(
            input: InputStream,
            charset: Charset,
            contentType: MediaType,
            extensionRegistry: ExtensionRegistry,
            builder: Message.Builder
        ) {
            if (contentType.isCompatibleWith(MediaType.APPLICATION_JSON)) {
                val reader = InputStreamReader(input, charset)
                parser.merge(reader, builder)
            } else {
                throw HttpMessageConversionException(
                    "protobuf-java-util does not support parsing $contentType"
                )
            }
        }

        @Throws(IOException::class, HttpMessageConversionException::class)
        override fun print(message: Message, output: OutputStream, contentType: MediaType, charset: Charset) {
            if (contentType.isCompatibleWith(MediaType.APPLICATION_JSON)) {
                val writer = OutputStreamWriter(output, charset)
                printer.appendTo(message, writer)
                writer.flush()
            } else {
                throw HttpMessageConversionException(
                    "protobuf-java-util does not support printing $contentType"
                )
            }
        }
    }

    companion object {
        private val logger = LogManager.getLogger()

        /**
         * The default charset used by the converter.
         */
        val DEFAULT_CHARSET = StandardCharsets.UTF_8

        /**
         * The media-type for protobuf `application/x-protobuf`.
         */
        val PROTOBUF = MediaType("application", "protobuf")
        val X_PROTOBUF = MediaType("application", "x-protobuf")

        /**
         * The HTTP header containing the protobuf schema.
         */
        const val X_PROTOBUF_SCHEMA_HEADER = "X-Protobuf-Schema"

        /**
         * The HTTP header containing the protobuf message.
         */
        const val X_PROTOBUF_MESSAGE_HEADER = "X-Protobuf-Message"
        private val METHOD_CACHE: MutableMap<Class<*>, Method> = ConcurrentReferenceHashMap()
    }
}
