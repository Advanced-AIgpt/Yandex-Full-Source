package ru.yandex.alice.paskill.dialogovo.service.api

import ru.yandex.alice.paskill.dialogovo.domain.FeedbackMark
import java.util.Optional

/**
 * methods throw [ApiException]
 */
interface ApiService {
    fun getFeedbackMark(userTicket: String, skillId: String, deviceUuid: String): FeedbackMark?

    fun getFeedbackMarkO(userTicket: String, skillId: String, deviceUuid: String): Optional<FeedbackMark>? =
        Optional.ofNullable(getFeedbackMark(userTicket, skillId, deviceUuid))

    fun putFeedbackMark(userTicket: String, skillId: String, mark: FeedbackMark, deviceUuid: String)
}
