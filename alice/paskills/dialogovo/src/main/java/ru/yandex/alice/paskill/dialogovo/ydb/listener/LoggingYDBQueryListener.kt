package ru.yandex.alice.paskill.dialogovo.ydb.listener

import com.yandex.ydb.core.Status
import org.apache.logging.log4j.LogManager
import org.springframework.stereotype.Component
import java.time.Duration

@Component
internal open class LoggingYDBQueryListener : YDBQueryListener {
    private val logger = LogManager.getLogger("ru.yandex.ydb.query")
    private val logWarnDuration = Duration.ofSeconds(1)

    override fun onQueryStarted(queryInfo: String) {}

    override fun onRetry(queryInfo: String, retryNumber: Int) {}

    override fun onQuerySuccess(queryInfo: String, duration: Duration) {
        if (duration > logWarnDuration) {
            logger.warn("Query [{}] took {}ms (LONG)", queryInfo, duration.toMillis())
        } else {
            logger.debug("Query [{}] took {}ms", queryInfo, duration.toMillis())
        }
    }

    override fun onQueryFailed(queryInfo: String, duration: Duration, status: Status?) {
        logger.error(
            "Failed to execute query [{}] took {}ms" +
                status?.let { " status: {$it}" },
            queryInfo, duration.toMillis()
        )
    }
}
