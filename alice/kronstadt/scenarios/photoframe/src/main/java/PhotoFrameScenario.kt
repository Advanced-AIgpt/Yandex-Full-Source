package ru.yandex.alice.paskill.dialogovo.scenarios.photoframe

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.BaseRunResponse
import ru.yandex.alice.kronstadt.core.DivRenderData
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.RelevantResponse
import ru.yandex.alice.kronstadt.core.RunOnlyResponse
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfo
import ru.yandex.alice.kronstadt.core.convert.response.ToProtoContext
import ru.yandex.alice.kronstadt.core.directive.AddGalleryCardDirective
import ru.yandex.alice.kronstadt.core.domain.converters.ScreenSaverConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.ScreenSaverScenarioData
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeaserConfigData
import ru.yandex.alice.kronstadt.core.domain.converters.TeaserPreviewDataConverter
import ru.yandex.alice.kronstadt.core.domain.scenariodata.teasers.TeasersPreviewData
import ru.yandex.alice.kronstadt.core.layout.Layout
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoargScene
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.protos.data.scenario.Data.TScenarioData

const val GET_PHOTO_INTENT = "alice_scenarios.get_photo_frame"
const val GET_PREVIEW_INTENT = "alice_scenarios.get_photo_preview"
const val NUM_PHOTOS_TO_ONE_GALLERY = 20

const val ALICE_CENTAUR_COLLECT_CARDS = "alice.centaur.collect_cards"
const val ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW = "alice.centaur.collect_teasers_preview"

// exp format photoframe_categories_with_num=nature:36,other:7
const val PHOTOFRAME_CATEGORIES_WITH_NUM_EXP = "photoframe_categories_with_num="
const val S3_PHOTO_PATH = "https://dialogs.s3.yandex.net/smart_displays/photoframe/"
const val PHOTO_FRAME_PHOTOS_POOL_SIZE = 35
const val TEASER_TYPE = "PhotoFrame";
const val TEASER_NAME = "Красивые картиночки"
const val TEASER_SETTINGS_EXP = "teaser_settings"

@Component
class PhotoFrameScenario :
    AbstractNoStateScenario(ScenarioMeta("photo_frame", "PhotoFrame", "photo_frame", useDivRenderer = true)) {

    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onFrame(ALICE_CENTAUR_COLLECT_CARDS) {
            scene<GetPhotoScene>()
        }
        onFrame(ALICE_CENTAUR_COLLECT_TEASERS_PREVIEW) {
            if (request.hasExperiment(TEASER_SETTINGS_EXP)) {
                scene<GetPreviewScene>()
            } else null
        }
    }
}

@Component
class GetPhotoScene(
    private val screenSaverConverter: ScreenSaverConverter
) : AbstractNoargScene<Any>("show_screen_saver") {

    override fun render(request: MegaMindRequest<Any>): RelevantResponse<Any> {
        val randomPhotoIds =
            parsePhotoIds(
                request.getExperimentsWithCommaSeparatedListValues(
                    PHOTOFRAME_CATEGORIES_WITH_NUM_EXP,
                    listOf()
                )
            )
                .shuffled(request.random)
                .subList(0, NUM_PHOTOS_TO_ONE_GALLERY)

        return RunOnlyResponse(layout = Layout(
            shouldListen = false, outputSpeech = null, directives = addPhotosToGallery(randomPhotoIds)
        ),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = GET_PHOTO_INTENT),
            isExpectsRequest = false,
            renderData = randomPhotoIds.map {
                DivRenderData(
                    cardId(it), TScenarioData.newBuilder().setScreenSaverData(
                        screenSaverConverter.convert(
                            ScreenSaverScenarioData(imageUrl(it)),
                            ToProtoContext()
                        )
                    ).build()
                )
            })
    }

    private fun addPhotosToGallery(
        randomPhotoIds: List<String>
    ): List<AddGalleryCardDirective> = randomPhotoIds.map { addGalleryCardDirective(it) }

    private fun addGalleryCardDirective(
        photoId: String
    ) = AddGalleryCardDirective(
        cardId = cardId(photoId),
        teaserConfigData = TeaserConfigData(
            teaserType = TEASER_TYPE
        )
    )

    private fun cardId(photoId: String) = "screen_saver_card_$photoId"
}

@Component
class GetPreviewScene(
    private val teaserPreviewDataConverter: TeaserPreviewDataConverter
) : AbstractNoargScene<Any>("screen_saver_preview") {
    override fun render(request: MegaMindRequest<Any>): BaseRunResponse<Any>? {
        val randomPhotoId =
            parsePhotoIds(
                request.getExperimentsWithCommaSeparatedListValues(
                    PHOTOFRAME_CATEGORIES_WITH_NUM_EXP,
                    listOf()
                )
            )
                .shuffled(request.random)
                .first()
        val teaserPreviewData = TeasersPreviewData(
            teaserPreviews = listOf(
                TeasersPreviewData.TeaserPreview(
                    teaserName = TEASER_NAME,
                    teaserConfigData = TeaserConfigData(TEASER_TYPE),
                    previewScenarioData = ScreenSaverScenarioData(imageUrl(randomPhotoId))

                )
            )
        )
        return RunOnlyResponse(
            layout = Layout(shouldListen = false, outputSpeech = null),
            state = null,
            analyticsInfo = AnalyticsInfo(intent = GET_PREVIEW_INTENT),
            isExpectsRequest = false,
            scenarioData = TScenarioData.newBuilder()
                .setTeasersPreviewData(teaserPreviewDataConverter.convert(teaserPreviewData, ToProtoContext()))
                .build()

        )
    }
}

fun parsePhotoIds(catsWithNum: List<String>): List<String> {
    return catsWithNum.let { catsWithNums ->
        if (catsWithNums.isEmpty())
            (1..PHOTO_FRAME_PHOTOS_POOL_SIZE).map { "nature$it" }.toList()
        else
            catsWithNums.flatMap {
                it.split(":")
                    .let { catWithNum ->
                        (1..catWithNum[1].toInt()).map { index -> catWithNum[0] + index }.toList()
                    }
            }
    }
}

fun imageUrl(photoId: String): String {
    return "${S3_PHOTO_PATH}$photoId.png"
}
