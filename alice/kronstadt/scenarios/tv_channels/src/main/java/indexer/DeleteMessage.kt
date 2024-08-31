package ru.yandex.alice.kronstadt.scenarios.tv_channels.indexer

import com.fasterxml.jackson.annotation.JsonProperty

class DeleteMessage(
    uri: String,
    override val prefix: Int = 1,
) : SaasMessage {
    override val action: SaasAction = SaasAction.delete

    @JsonProperty("docs")
    val docs = listOf(Document(uri))

    class Document(val url: String)
}
