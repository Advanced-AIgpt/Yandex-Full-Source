package ru.yandex.alice.social.sharing

import java.util.*

fun getQueryString(pathWithQuery: String): String {
    val parts = pathWithQuery.split("?", limit = 2)
    return if (parts.size == 2) parts[1] else ""
}

/**
 * Split request query string into url-encoded key-value pairs.
 */
fun splitQueryString(queryString: String): TreeMap<String, String> {
    val params: TreeMap<String, String> = TreeMap()
    for (pairString in queryString.split("&")) {
        val parts = pairString.split("=", limit = 2)
        val key = parts[0]
        val value = if (parts.size == 1) "" else parts[1]
        params[key] = value
    }
    return params
}
