package ru.yandex.alice.social.sharing.document

class MockDocumentProvider: DocumentProvider() {

    val candidates: MutableMap<String, Document> = mutableMapOf()
    val documents: MutableMap<String, Document> = mutableMapOf()

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
        val candidate = candidates.get(id)
        if (candidate == null) {
            throw CandidateNotFoundException(id)
        }
        documents[id] = candidate
    }

    fun clear() {
        candidates.clear()
        documents.clear()
    }
}
