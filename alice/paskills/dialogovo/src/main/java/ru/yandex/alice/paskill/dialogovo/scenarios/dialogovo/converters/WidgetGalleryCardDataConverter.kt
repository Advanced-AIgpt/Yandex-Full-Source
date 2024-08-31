package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.converters

import com.google.protobuf.Any
import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.ScenarioData
import ru.yandex.alice.megamind.protos.common.FrameProto.TTypedSemanticFrame
import ru.yandex.alice.paskill.dialogovo.domain.ActivationSourceType
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.MainScreenSkillCardData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.SkillInfoData
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.domain.WidgetGallerySkillCardData
import ru.yandex.alice.paskill.dialogovo.semanticframes.FixedActivate
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen.TCentaurWidgetCardData
import ru.yandex.alice.protos.data.scenario.centaur.MainScreen.TCentaurWidgetCardData.TExternalSkillCardData

@Component
open class WidgetGalleryCardDataConverter : ToProtoConverter<ScenarioData, TCentaurWidgetCardData> {
    override fun convert(src: ScenarioData, ctx: ToProtoContext): TCentaurWidgetCardData {
        when (src) {
            is MainScreenSkillCardData -> return convertMainScreenCardData(src)
            is WidgetGallerySkillCardData -> return convertWidgetGalleryCardData(src)
        }
        throw RuntimeException("Can't find converter for scenario data of type: " + src.javaClass.name)
    }

    private fun convertMainScreenCardData(src: MainScreenSkillCardData): TCentaurWidgetCardData {
        val skillResponse = src.skillResponse
        val mainScreenDataBuilder = TExternalSkillCardData.TMainScreenData.newBuilder()
        if (skillResponse != null) {
            skillResponse.title?.let { mainScreenDataBuilder.title = it }
            skillResponse.text?.let { mainScreenDataBuilder.text = it }
            skillResponse.imageUrl?.let { mainScreenDataBuilder.imageUrl = it }
            if (skillResponse.buttons.isNotEmpty()) {
                mainScreenDataBuilder.addAllButtons(skillResponse.buttons.map {
                    convertButton(it)
                })
            }
        }
        return convertCentaurWidgetCardDataTypedAction(
            TExternalSkillCardData.newBuilder()
                .setMainScreenData(mainScreenDataBuilder)
                .setSkillInfo(convertSkillInfo(src.skillInfoData))
                .build(),
            src.skillInfoData.skillId,
            src
        )
    }

    private fun convertWidgetGalleryCardData(src: WidgetGallerySkillCardData): TCentaurWidgetCardData {
        val externalSkillCardData = TExternalSkillCardData.newBuilder()
            .setWidgetGalleryData(TExternalSkillCardData.TWidgetGalleryData.newBuilder())
            .setSkillInfo(convertSkillInfo(src.skillInfoData))
            .build()
        return convertCentaurWidgetCardData(externalSkillCardData, src.skillInfoData.skillId)
    }

    private fun convertSkillInfo(skillInfoData: SkillInfoData): TExternalSkillCardData.TSkillInfo {
        return TExternalSkillCardData.TSkillInfo.newBuilder().setSkillId(skillInfoData.skillId)
            .setLogo(skillInfoData.logo)
            .setName(skillInfoData.name).build()
    }

    private fun convertButton(button: MainScreenSkillCardData.Button) =
        TExternalSkillCardData.TMainScreenData.TButton.newBuilder()
            .setText(button.title)
            .setPayload(button.payload ?: "")
            .build()


    private fun convertCentaurWidgetCardData(
        externalSkillCardData: TExternalSkillCardData,
        id: String
    ): TCentaurWidgetCardData {
        return TCentaurWidgetCardData.newBuilder()
            .setId(id)
            .setExternalSkillCardData(externalSkillCardData)
            .setAction(convertAction(id))
            .build()
    }

    private fun convertCentaurWidgetCardDataTypedAction(
        externalSkillCardData: TExternalSkillCardData,
        id: String,
        mainScreenSkillCardData: MainScreenSkillCardData
    ): TCentaurWidgetCardData {
        return TCentaurWidgetCardData.newBuilder()
            .setId(id)
            .setExternalSkillCardData(externalSkillCardData)
            .setAction(convertAction(id))
            .setTypedAction(getSkillActivateTypedAction(mainScreenSkillCardData))
            .build()
    }

    private fun getSkillActivateTypedAction(mainScreenSkillCardData: MainScreenSkillCardData): Any {
        val builder = TTypedSemanticFrame.newBuilder()
        FixedActivate(
                skillId = mainScreenSkillCardData.skillInfoData.skillId,
                payload = mainScreenSkillCardData.skillResponse?.tapAction?.payload,
                activationCommand = mainScreenSkillCardData.skillResponse?.tapAction?.activationCommand,
                activationSourceType = ActivationSourceType.WIDGET_GALLERY
            ).writeTypedSemanticFrame(builder)
        return Any.pack(builder.build())
    }

    private fun convertAction(id: String) = "@@mm_deeplink#$id"
}
