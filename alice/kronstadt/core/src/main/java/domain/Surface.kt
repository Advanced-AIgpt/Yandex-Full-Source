package ru.yandex.alice.kronstadt.core.domain

import com.fasterxml.jackson.annotation.JsonValue
import java.util.Optional

enum class Surface(@field:JsonValue val code: String, val isImplicit: Boolean, val isHasScreen: Boolean) {
    DESKTOP("desktop", true, true),
    MOBILE("mobile", true, true),
    AUTO("auto", false, false),
    NAVIGATOR("navigator", false, false),
    QUASAR("station", false, false),
    WATCH("watch", false, false),
    SMART_TV("smart_tv", false, false),
    CENTAUR("centaur", false, true);

    companion object {
        private val CODE_MAP: Map<String, Surface> = values().associateBy { it.code }

        @JvmStatic
        fun byCode(code: String): Optional<Surface> {
            return Optional.ofNullable(CODE_MAP[code])
        }
    }
}
