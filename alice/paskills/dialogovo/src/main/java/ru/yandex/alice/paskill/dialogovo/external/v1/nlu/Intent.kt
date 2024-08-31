package ru.yandex.alice.paskill.dialogovo.external.v1.nlu

import com.fasterxml.jackson.annotation.JsonIgnore

data class Intent @JvmOverloads constructor(
    // intent is serialized to hash map's key in request to webhook
    @JsonIgnore val name: String,
    val slots: Map<String, NluEntity> = mapOf()
) {

    fun withName(newName: String): Intent = Intent(newName, slots)

    fun withoutAdditionalValues(): Intent =
        Intent(name, slots.mapValues { (_, value) -> value.withoutAdditionalValues() })
}
