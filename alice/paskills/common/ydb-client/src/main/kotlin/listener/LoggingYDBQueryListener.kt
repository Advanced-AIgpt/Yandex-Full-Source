package ru.yandex.alice.paskills.common.ydb.listener

import com.yandex.ydb.core.Status
import org.apache.logging.log4j.LogManager
import java.time.Duration

class LoggingYDBQueryListener(private val logWarnDuration: Duration) : YDBQueryListener {
    override fun onQueryStarted(queryName: String, duration: Duration) {

        if (duration > logWarnDuration) {
            logger.warn("Query [{}] waited {}ms before execution (LONG)", queryName, duration.toMillis())
        } else {
            logger.debug("Query [{}] waited {}ms before execution", queryName, duration.toMillis())
        }
    }

    override fun onRetry(queryName: String, retryNumber: Int) {}
    override fun onQuerySuccess(queryName: String, duration: Duration) {
        if (duration > logWarnDuration) {
            logger.warn("Query [{}] took {}ms (LONG)", queryName, duration.toMillis())
        } else {
            logger.debug("Query [{}] took {}ms", queryName, duration.toMillis())
        }
    }

    override fun onQueryFailed(queryName: String, duration: Duration, status: Status?) {
        val statusStr = if (status != null) " status: {$status}" else ""
        logger.error(
            "Failed to execute query [{}] took {}ms{}", queryName, duration.toMillis(), statusStr
        )
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
