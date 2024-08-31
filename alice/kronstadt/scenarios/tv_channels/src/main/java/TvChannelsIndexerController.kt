package ru.yandex.alice.kronstadt.scenarios.tv_channels

import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RequestHeader
import org.springframework.web.bind.annotation.RestController
import javax.validation.Valid
import javax.validation.constraints.NotBlank
import javax.validation.constraints.NotEmpty

@RestController
class TvChannelsIndexerController(
    private val indexerService: IndexerService,
) {
    @PostMapping("/tv-indexer/index-cable-channels")
    fun indexCableChannels(
        @NotBlank @RequestHeader("X-QuasarDeviceID") quasarDeviceId: String,
        @Valid @RequestBody request: IndexCableChannelsRequest,
    ) {
        indexerService.indexAllDocuments(quasarDeviceId, request.timestamp, request.channels)
        indexerService.cleanOldDocuments(quasarDeviceId, request.timestamp)
    }

    data class IndexCableChannelsRequest(
        @NotEmpty
        val channels: List<Channel>,
        val timestamp: Long
    )
}
