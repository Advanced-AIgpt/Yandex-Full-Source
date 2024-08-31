package ru.yandex.alice.social.sharing.apphost.handlers.document.ydb

import com.google.common.primitives.Longs
import ru.yandex.alice.social.sharing.proto.SocialSharingApi
import java.security.MessageDigest
import kotlin.random.Random

private val ALPHABET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
private val DOCUMENT_ID_SIZE = 14
private val DOCUMENT_ID_PATTERN = "[a-zA-Z]{${DOCUMENT_ID_SIZE},}".toRegex()

fun generateDocumentId(request: SocialSharingApi.TCreateCandidateRequest): String {
    val md5 = MessageDigest.getInstance("md5")
    md5.update(request.createSocialLinkDirective.scenarioSharePage.toByteArray())
    val documentSeed = Longs.fromByteArray(md5.digest())
    val random = Random(request.randomSeed xor documentSeed)
    val sb = StringBuilder()
    for (position in 1..DOCUMENT_ID_SIZE) {
        sb.append(ALPHABET[random.nextInt(ALPHABET.length)])
    }
    return sb.toString()
}

fun isValidDocumentId(s: String): Boolean = DOCUMENT_ID_PATTERN.matchEntire(s) != null
