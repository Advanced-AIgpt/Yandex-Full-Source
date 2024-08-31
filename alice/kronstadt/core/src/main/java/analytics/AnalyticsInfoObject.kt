package ru.yandex.alice.kronstadt.core.analytics

import ru.yandex.alice.megamind.protos.scenarios.AnalyticsInfo.TAnalyticsInfo.TObject

open class AnalyticsInfoObject(id: String, name: String, humanReadable: String) :
    BaseAnalyticsInfoItem(id, name, humanReadable) {

    fun toProto(): TObject {
        val protoBuilder: TObject.Builder = TObject.newBuilder()
            .setId(id)
            .setName(name)
            .setHumanReadable(humanReadable)

        return fillProtoField(protoBuilder).build()
    }

    protected open fun fillProtoField(protoBuilder: TObject.Builder): TObject.Builder = protoBuilder

    override fun toString(): String {
        return "AnalyticsInfoObject(id='$id', name='$name', humanReadable='$humanReadable')"
    }
}
