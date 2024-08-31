package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement
import java.util.UUID

data class UserAgreementDb(val id: UUID, val skillId: UUID, val order: Int, val name: String, val url: String) {
    fun toUserAgreement(): UserAgreement {
        return UserAgreement(id, skillId, name, order, url)
    }
}
