package ru.yandex.alice.paskill.dialogovo.service.state

import com.fasterxml.jackson.annotation.JsonProperty

data class TestStateStruct(
    @param:JsonProperty("sessions")
    val sessions: List<SessionStateEntity> = emptyList(),

    @param:JsonProperty("users")
    val users: List<UserStateEntity> = emptyList(),

    @param:JsonProperty("applications")
    val applications: List<ApplicationStateEntity> = emptyList()
)
