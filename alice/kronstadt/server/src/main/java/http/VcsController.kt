package ru.yandex.alice.kronstadt.server.http

import org.springframework.web.bind.annotation.GetMapping
import org.springframework.web.bind.annotation.RestController
import ru.yandex.alice.kronstadt.core.VersionProvider

@RestController
internal class VcsController(private val versionProvider: VersionProvider) {
    @get:GetMapping(value = ["/version_json"])
    val vcsVersionJson: VcsVersionJsonResponse
        get() = VcsVersionJsonResponse(versionProvider.branch, versionProvider.tag)

    data class VcsVersionJsonResponse(val branch: String?, val tag: String?)
}
