package ru.yandex.alice.paskill.dialogovo.processor

import java.time.Duration

enum class GeolocationSharingOptions(
    val title: String,
    val code: String,
    val isOptionAllowSharing: Boolean,
    val periodForAllowedSharing: Duration,
    val isAvailableInProduction: Boolean
) {
    FIVE_MINUTES("Разрешить на 5 минут", "five_minutes", true, Duration.ofMinutes(5), false),
    ONE_HOUR("Разрешить на 1 час", "one_hour", true, Duration.ofHours(1), true),
    ONE_DAY("Разрешить на 1 день", "one_day", true, Duration.ofDays(1), true),
    DONT_ALLOW("Не разрешать", "do_not_allow", false, Duration.ofSeconds(0), true);

    val allowedPeriodAsMillis: Long
        get() = periodForAllowedSharing.toMillis()

    companion object {
        private val codeMap = values().associateBy { it.code }

        @JvmStatic
        fun byCode(code: String): GeolocationSharingOptions? = codeMap[code]
    }
}
