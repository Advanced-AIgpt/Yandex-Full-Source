package ru.yandex.alice.kronstadt.core.response

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.alice.kronstadt.core.convert.response.ServerDirectiveConverter
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.server.MementoChangeUserObjectsDirective
import ru.yandex.alice.kronstadt.core.directive.server.UserConfig
import ru.yandex.alice.memento.proto.MementoApiProto.EConfigKey
import ru.yandex.alice.memento.proto.UserConfigsProto.TSmartTvTemplateInfo

class ServerDirectiveConverterTest {
    val directiveConverter = ServerDirectiveConverter()

    @Test
    fun convertMementoChangeUserObjectsDirectiveOk() {
        val ctx = ToProtoContext()
        val expectedShowCount = 100500
        val expectedLastAppearanceTime = 100501L
        val serverDirective = MementoChangeUserObjectsDirective(
            mapOf(
                EConfigKey.CK_TANDEM_PROMO_TEMPLATE_INFO to UserConfig(
                    TSmartTvTemplateInfo.newBuilder().apply {
                        lastAppearanceTime = expectedLastAppearanceTime
                        showCount = expectedShowCount
                    }.build()
                )
            )
        )

        val convertResult = directiveConverter.convert(serverDirective, ctx)
        assertEquals(
            EConfigKey.CK_TANDEM_PROMO_TEMPLATE_INFO,
            convertResult.mementoChangeUserObjectsDirective.userObjects.userConfigsList[0].key
        )
        val templateInfoRaw =
            convertResult.mementoChangeUserObjectsDirective.userObjects.userConfigsList[0].value
        val templateInfo = templateInfoRaw.unpack(TSmartTvTemplateInfo::class.java)
        assertEquals(expectedShowCount, templateInfo.showCount)
        assertEquals(expectedLastAppearanceTime, templateInfo.lastAppearanceTime)
    }
}
