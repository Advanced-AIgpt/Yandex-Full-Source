package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor

data class UserAgreementsAcceptedEvent(
    val userAgreementIds: Set<String>
)

object UserAgreementsRejectedEvent
