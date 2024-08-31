package ru.yandex.alice.paskill.dialogovo.service.recommender

interface RecommenderService {
    fun search(
        cardName: RecommenderCardName,
        query: String? = null,
        attributes: RecommenderRequestAttributes
    ): RecommenderResponse
}
