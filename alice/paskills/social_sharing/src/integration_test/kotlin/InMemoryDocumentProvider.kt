package ru.yandex.alice.social.sharing

import ru.yandex.alice.social.sharing.document.CandidateNotFoundException
import ru.yandex.alice.social.sharing.document.Document
import ru.yandex.alice.social.sharing.document.DocumentProvider

abstract class TestDocumentProvider: DocumentProvider() {
    abstract fun clear()
}

class InMemoryDocumentProvider(
    private val candidates: MutableMap<String, Document> = mutableMapOf(),
    private val documents: MutableMap<String, Document> = mutableMapOf(),
): TestDocumentProvider() {

    constructor(documents: List<Document>): this(
        mutableMapOf(),
        documents.map { it.id to it }.toMap().toMutableMap(),
    )

    override fun get(id: String): Document? {
        return documents[id]
    }

    override fun upsertDocument(document: Document) {
        documents[document.id] = document
    }

    override fun createCandidate(document: Document) {
        candidates[document.id] = document
    }

    override fun commitCandidate(id: String) {
        val document = candidates[id]
        if (document == null) {
            throw CandidateNotFoundException(id)
        }
        upsertDocument(document)
    }

    override fun clear() {
        candidates.clear()
        documents.clear()
    }

}
