package ru.yandex.alice.kronstadt.core.utils

import org.springframework.util.StringUtils

fun String?.prepend(prefix: String?, separator: String, capitalizeSelf: Boolean): String? {
    if (prefix == null) {
        return this
    }
    val capitalizedThis = if (capitalizeSelf && this != null) StringUtils.capitalize(this) else this
    return if (!this.isNullOrEmpty()) (prefix + separator + capitalizedThis) else prefix
}

fun String?.append(suffix: String?, separator: String): String? {
    if (suffix.isNullOrEmpty()) {
        return this
    }
    return if (!this.isNullOrEmpty()) this + separator + suffix else suffix
}
