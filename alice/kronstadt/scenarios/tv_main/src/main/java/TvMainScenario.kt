package ru.yandex.alice.kronstadt.scenarios.tvmain

import org.springframework.stereotype.Component
import ru.yandex.alice.kronstadt.core.MegaMindRequest
import ru.yandex.alice.kronstadt.core.ScenarioMeta
import ru.yandex.alice.kronstadt.core.domain.ClientInfo
import ru.yandex.alice.kronstadt.core.scenario.AbstractNoStateScenario
import ru.yandex.alice.kronstadt.core.scenario.SelectedScene
import ru.yandex.alice.megamind.protos.common.FrameProto
import ru.yandex.alice.megamind.protos.common.FrameProto.TGetVideoGalleriesSemanticFrame
import ru.yandex.alice.megamind.protos.common.FrameProto.TGetVideoGallerySemanticFrame
import ru.yandex.alice.protos.data.video.VideoScenesProto
import ru.yandex.kronstadt.alice.scenarios.tv_main.proto.TVideoGetGallerySceneArgs

val SCENARIO_META = ScenarioMeta("tv_main")

@Component
class TvMainScenario : AbstractNoStateScenario(SCENARIO_META) {
    override fun selectScene(request: MegaMindRequest<Any>): SelectedScene.Running<Any, *>? = request.handle {
        onClient(ClientInfo::isTvDevice) {
            onFrame("alice.video.get_gallery") { frame ->
                if (frame.typedSemanticFrame?.hasGetVideoGallerySemanticFrame() != true) return@onFrame null
                val typedFrame = frame.typedSemanticFrame!!.getVideoGallerySemanticFrame
                sceneWithArgs(GetGalleryScene::class, toGetGallerySceneArgs(typedFrame))
            }
            onFrame("alice.video.get_galleries") { frame ->
                if (frame.typedSemanticFrame?.hasGetVideoGalleries() != true) return@onFrame null
                val typedFrame = frame.typedSemanticFrame!!.getVideoGalleries
                sceneWithArgs(GetGalleriesScene::class, typedFrame)
            }
        }
    }
}

private fun toGetGallerySceneArgs(sf: TGetVideoGallerySemanticFrame): TVideoGetGallerySceneArgs {
    return TVideoGetGallerySceneArgs.newBuilder().apply {
        id = sf.id.stringValue
        offset = sf.offset.numValue
        limit = if (sf.hasLimit()) sf.limit.numValue else 12

        if (sf.hasCacheHash()) {
            cacheHash = sf.cacheHash.stringValue
        }
        if (sf.hasParentFromScreenId()) {
            parentFromScreenId = sf.parentFromScreenId.stringValue
        }
        if (sf.hasFromScreenId()) {
            fromScreenId = sf.fromScreenId.stringValue
        }
        if (sf.hasCarouselPosition()) {
            carouselPosition = sf.carouselPosition.numValue
        }
        if (sf.hasCarouselTitle()) {
            carouselTitle = sf.carouselTitle.stringValue
        }
        if (sf.hasKidModeEnabled()) {
            kidModeEnabled = sf.kidModeEnabled.boolValue
        }
        if (sf.hasRestrictionAge()) {
            restrictionAge = sf.restrictionAge.stringValue
        }
        if (sf.hasSelectedTags()) {
            addAllSelectedTags(sf.selectedTags.catalogTags.catalogTagValueList)
        }
    }.build()
}
