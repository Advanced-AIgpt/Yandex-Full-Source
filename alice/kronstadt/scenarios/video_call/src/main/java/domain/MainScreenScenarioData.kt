package ru.yandex.alice.kronstadt.scenarios.video_call.domain

import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData

data class MainScreenScenarioData(
    val providerData: ProviderData,
) : ScenarioData {

    sealed interface ProviderData

    data class TelegramCardData(
        val loggedIn: Boolean,
        val contactsUploaded: Boolean = false,
        val userId: String? = null,
        val favoriteContactData: List<FavoriteContactData> = listOf()
    ) : ProviderData

    data class FavoriteContactData(
        val displayName: String,
        val userId: String,
        val lookupKey: String
    )

    companion object {
        val LOGGED_OUT_MAIN_SCREEN_DATA = MainScreenScenarioData(TelegramCardData(loggedIn = false))
    }
}
