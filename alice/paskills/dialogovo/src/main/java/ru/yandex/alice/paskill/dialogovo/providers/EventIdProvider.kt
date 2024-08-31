package ru.yandex.alice.paskill.dialogovo.providers

import org.springframework.stereotype.Component
import java.util.UUID

interface EventIdProvider {
    fun generateEventId(): UUID
}

@Component
internal class EventIdProviderImpl : EventIdProvider {
    override fun generateEventId(): UUID = UUID.randomUUID()
}
