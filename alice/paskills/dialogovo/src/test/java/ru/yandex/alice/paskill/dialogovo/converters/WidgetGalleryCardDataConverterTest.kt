package ru.yandex.alice.paskill.dialogovo.converters

import org.junit.jupiter.api.Assertions
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters.WidgetGalleryCardDataConverter
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.MainScreenSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen

class WidgetGalleryCardDataConverterTest {
    private val widgetGalleryCardDataConverter = WidgetGalleryCardDataConverter()
    private val skillInfo = SkillInfoData("skill_name", "skill_logo", "skill_id")
    private val mainScreenCardData = MainScreenSkillCardData(
        skillInfoData = skillInfo,
        skillResponse = null
    )

    @Test
    fun `Empty MainScreenData when empty response body`() {
        val convertedResult = widgetGalleryCardDataConverter.convert(mainScreenCardData, ToProtoContext())
        assertTrue(convertedResult.hasExternalSkillCardData())
        assertTrue(convertedResult.externalSkillCardData.hasMainScreenData())
        assertEquals(
            convertedResult.externalSkillCardData.mainScreenData,
            MainScreen.TCentaurWidgetCardData.TExternalSkillCardData.TMainScreenData.newBuilder().build()
        )
    }

    @Test
    fun `Throw exception with wrong scenario data`() {
        val scenarioData = object : ScenarioData {}
        Assertions.assertThrows(RuntimeException::class.java) {
            widgetGalleryCardDataConverter.convert(
                scenarioData,
                ToProtoContext()
            )
        }
    }

    @Test
    fun `Set correct action id`() {
        val convertedResult = widgetGalleryCardDataConverter.convert(mainScreenCardData, ToProtoContext())
        assertEquals(convertedResult.action, "@@mm_deeplink#skill_id")
    }
}
