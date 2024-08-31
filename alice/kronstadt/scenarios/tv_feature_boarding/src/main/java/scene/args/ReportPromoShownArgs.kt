package ru.yandex.alice.kronstadt.scenarios.tv_feature_boarding.scene.args

import ru.yandex.alice.megamind.protos.common.FrameProto.TTvChosenTemplateSlot

data class ShowCounts(
    val tandemShowCount: Int
)

data class ReportPromoShownArgs(
    val chosenTemplateCase: TTvChosenTemplateSlot.ValueCase,
    val chosenTemplateName: String,
    val showTemplateTimestamp: Long,
    val showCounts: ShowCounts
)
