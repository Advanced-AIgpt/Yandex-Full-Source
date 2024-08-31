package ru.yandex.alice.kronstadt.scenarios.tv_channels.search

import org.springframework.http.RequestEntity
import org.springframework.http.ResponseEntity

interface SaasSearchClient {
    fun createSearchRequest(searchQuery: String): RequestEntity<*>
    fun parseResult(response: ResponseEntity<SearchResponse>): List<FoundDocument>
    fun search(searchQuery: String): List<FoundDocument>
}
