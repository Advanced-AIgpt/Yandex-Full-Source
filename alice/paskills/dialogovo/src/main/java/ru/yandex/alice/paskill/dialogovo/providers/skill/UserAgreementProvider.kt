package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement
import java.util.UUID

interface UserAgreementProvider {
    fun getPublishedUserAgreements(skillId: UUID): List<UserAgreement>
    fun getDraftUserAgreements(skillId: UUID): List<UserAgreement>
}
