package ru.yandex.alice.social.sharing

import org.apache.logging.log4j.LogManager
import ru.yandex.alice.social.sharing.document.QueryParams
import java.net.URLEncoder
import java.util.*
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

object HmacSignature {

    private val logger = LogManager.getLogger()

    @JvmStatic
    private val HMAC_SHA256 = "HmacSHA256"

    fun validate(params: TreeMap<String, String>, secretBase64: String, signatureFromParams: ByteArray): Boolean {
        val signature: ByteArray = sign(params, secretBase64)
        return signature.isNotEmpty() && signature.contentEquals(signatureFromParams)
    }

    fun sign(params: TreeMap<String, String>, secretBase64: String): ByteArray {
        return sign(params, Base64.getDecoder().decode(secretBase64))
    }

    fun sign(params: TreeMap<String, String>, secret: ByteArray): ByteArray {
        return sign(params, SecretKeySpec(secret, HMAC_SHA256))
    }

    fun sign(params: TreeMap<String, String>, secret: SecretKeySpec): ByteArray {
        val dataString = params
            .filter { it.key != QueryParams.SIGNATURE }
            .map { it.key + "=" + it.value }
            .joinToString("&")
        logger.info("Signing params: {}", dataString)
        val dataBuffer = UTF_8.encode(dataString)
        val data = ByteArray(dataBuffer.remaining())
        dataBuffer.get(data)
        val hmac = Mac.getInstance(HMAC_SHA256)
        hmac.init(secret)
        val signature = hmac.doFinal(data)
        val stringSignature = Base64.getEncoder().encodeToString(signature)
        logger.info(
            "Calculated signature: {} ({} in url encoded format)",
            stringSignature,
            URLEncoder.encode(stringSignature, UTF_8)
        )
        return signature
    }

}
