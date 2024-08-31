package ru.yandex.alice.social.sharing.document

import java.net.URI

fun parseLinkId(path: String, prefix: String = "get_page"): String? {
    val cleanPath = URI(path).path
    val parts = cleanPath.split("/")
    var nextPartIsId = false
    for (part in parts) {
        if (part == prefix) {
            nextPartIsId = true;
        } else if (nextPartIsId) {
            return if (part != "") part else null
        }
    }
    return null
}
