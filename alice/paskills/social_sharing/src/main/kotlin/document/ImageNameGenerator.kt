package ru.yandex.alice.social.sharing.document

import ru.yandex.alice.social.sharing.UTF_8
import java.security.MessageDigest

fun interface ImageNameGenerator {
    fun get(url: String): String
}

private fun hex(bytes: ByteArray): String {
    val result = StringBuffer()
    for (b in bytes) {
        result.append(String.format("%02X", b))
    }
    return result.toString().toLowerCase()
}

val md5Generator = ImageNameGenerator { url ->
    val digest = MessageDigest.getInstance("MD5")
    digest.update(url.toByteArray(UTF_8))
    hex(digest.digest())
}
