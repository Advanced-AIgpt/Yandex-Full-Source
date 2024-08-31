package ru.yandex.alice.paskill.dialogovo.providers.skill

import ru.yandex.alice.paskill.dialogovo.domain.UserAgreement

interface SkillUserAgreementProvider {
    val skillUserAgreements: List<UserAgreement>
}
