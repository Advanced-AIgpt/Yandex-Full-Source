package ru.yandex.alice.paskill.dialogovo.service.abuse

internal interface AbuseService {
    fun checkAbuse(docs: List<AbuseDocument>): Map<String, String>
}
