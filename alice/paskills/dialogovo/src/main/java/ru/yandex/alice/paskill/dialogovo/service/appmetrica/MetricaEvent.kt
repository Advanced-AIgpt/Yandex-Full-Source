package ru.yandex.alice.paskill.dialogovo.service.appmetrica

import com.fasterxml.jackson.annotation.JsonInclude
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode

@JsonInclude(JsonInclude.Include.NON_NULL)
data class MetricaEvent(val name: String, val value: ObjectNode?) {

    constructor(name: String) : this(name, null)

    companion object TypedMetricaEvent {
        fun create(name: String, typeValue: String, mapper: ObjectMapper): MetricaEvent =
            MetricaEvent(name, mapper.valueToTree(MetricaTypeValue(typeValue)))

        data class MetricaTypeValue(val type: String)
    }
}
