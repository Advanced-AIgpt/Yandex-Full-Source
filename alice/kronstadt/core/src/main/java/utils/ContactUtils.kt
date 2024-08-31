package ru.yandex.alice.kronstadt.core.utils

import com.google.protobuf.ByteString
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.library.contacts.proto.Contacts
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.util.Base64
import java.util.zip.GZIPInputStream

object ContactUtils {

    private val logger = LogManager.getLogger(ContactUtils::class.java)

    private const val BUFFER_SIZE = 1024

    fun deserializeLookupKeyMapping(serialized: ByteString): Map<Int, String>? {
        return try {
            val decodedMapping = Base64.getUrlDecoder().decode(serialized.toByteArray())

            GZIPInputStream(ByteArrayInputStream(decodedMapping)).use { gzipInput ->
                ByteArrayOutputStream(decodedMapping.size).use { outputStream ->
                    val buffer = ByteArray(BUFFER_SIZE)
                    while (true) {
                        val byteCount = gzipInput.read(buffer)
                        if (byteCount <= 0) break
                        outputStream.write(buffer, 0, byteCount)
                    }
                    val result = outputStream.toByteArray()
                    Contacts.TLookupKeyMap.parseFrom(result)?.mappingList
                        ?.associateBy({ it.lookupIndex}, { it.lookupKey })
                }
            }
        } catch (e: Exception) {
            logger.error("Failed decompression lookup keys mapping:", e)
            return null
        }
    }
}