package ru.yandex.alice.paskill.dialogovo.megamind

import org.junit.jupiter.api.Assertions.assertFalse
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.domain.ClientInfo

internal class ClientInfoTest {
    @Test
    fun supportsDivCards() {
        assertTrue(
            ClientInfo(
                appId = "ru.yandex.mobile",
                platform = "ios",
                osVersion = "11.0",
                appVersion = "10",
            ).supportsDivCards()
        )
        assertTrue(
            ClientInfo(
                appId = "ru.yandex.mobile.search",
                platform = "android",
                appVersion = "17.10.1.291",
            )
                .supportsDivCards()
        )
        assertFalse(
            ClientInfo(
                appId = "yandex.auto",
                platform = "android",
                appVersion = "17.10.1.291",
            )
                .supportsDivCards()
        )
    }
}
