package ru.yandex.alice.kronstadt.core.analytics

import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TAction

open class AnalyticsInfoAction(id: String, name: String, humanReadable: String) :
    BaseAnalyticsInfoItem(id, name, humanReadable) {

    fun toProto(): TAction {
        val protoBuilder: TAction.Builder = TAction.newBuilder()
            .setId(id)
            .setName(name)
            .setHumanReadable(humanReadable)

        return fillProtoField(protoBuilder).build()
    }

    protected open fun fillProtoField(protoBuilder: TAction.Builder): TAction.Builder = protoBuilder

    override fun toString(): String {
        return "AnalyticsInfoAction(id='$id', name='$name', humanReadable='$humanReadable')"
    }
}
