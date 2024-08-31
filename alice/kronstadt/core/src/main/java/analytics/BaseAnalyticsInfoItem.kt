package ru.yandex.alice.kronstadt.core.analytics

/*
Base class for AnalyticsInfoObject and AnalyticsInfoAction as they have the same set of fields
 */
abstract class BaseAnalyticsInfoItem(val id: String, val name: String, val humanReadable: String) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as BaseAnalyticsInfoItem

        if (id != other.id) return false
        if (name != other.name) return false
        if (humanReadable != other.humanReadable) return false

        return true
    }

    override fun hashCode(): Int {
        var result = id.hashCode()
        result = 31 * result + name.hashCode()
        result = 31 * result + humanReadable.hashCode()
        return result
    }
}
