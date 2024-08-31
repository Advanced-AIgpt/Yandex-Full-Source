package ru.yandex.alice.paskill.dialogovo.domain

import com.fasterxml.jackson.annotation.JsonCreator
import java.util.UUID

data class UserAgreement(
    val id: UUID,
    val skillId: UUID,
    val name: String,
    val order: Int,
    val url: String
) : Comparable<UserAgreement> {

    @JsonCreator
    constructor(
        id: String,
        skillId: String,
        name: String,
        order: Int,
        url: String
    ) : this(
        id = UUID.fromString(id),
        skillId = UUID.fromString(skillId),
        name = name,
        order = order,
        url = url,
    )

    override fun compareTo(other: UserAgreement): Int {
        return this.order.compareTo(other.order)
    }
}
