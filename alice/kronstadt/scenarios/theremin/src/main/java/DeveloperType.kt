package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import ru.yandex.alice.megamind.protos.analytics.scenarios.dialogovo.Dialogovo.TSkillDeveloperType
import javax.annotation.Nullable

enum class DeveloperType(val value: String) {
    Yandex("yandex"),
    External("external");

    companion object {
        fun fromString(@Nullable developerType: String?): DeveloperType {
            return if ("yandex".equals(developerType)) {
                Yandex
            } else {
                External
            }
        }

        fun getProtoDeveloperType(source: DeveloperType): TSkillDeveloperType {
            return when (source) {
                Yandex -> TSkillDeveloperType.Yandex
                else -> TSkillDeveloperType.External
            }
        }
    }
}
