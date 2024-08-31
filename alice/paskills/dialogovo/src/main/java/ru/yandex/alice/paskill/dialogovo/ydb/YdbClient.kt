package ru.yandex.alice.paskill.dialogovo.ydb

import com.google.common.base.Stopwatch
import com.yandex.ydb.core.Result
import com.yandex.ydb.core.Status
import com.yandex.ydb.core.UnexpectedResultException
import com.yandex.ydb.core.utils.Async
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.SessionRetryContext
import com.yandex.ydb.table.query.DataQueryResult
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.settings.ExecuteDataQuerySettings
import com.yandex.ydb.table.settings.PrepareDataQuerySettings
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskill.dialogovo.ydb.listener.YDBQueryListener
import java.time.Duration
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.function.Function

class YdbClient(
    private val ctx: SessionRetryContext,
    private val listeners: List<YDBQueryListener>,
    private val poolSize: Int
) {
    private val defaultTimeout = Duration.ofSeconds(5)
    private val logger = LogManager.getLogger()

    /**
     * Prepare queries in session. This establishes connections warming them up and putting prepared query into cache
     * of at least some of session
     *
     * @param queries list of queries to prepare
     */
    fun warmUpSessionPool(queries: List<String>) {
        if (queries.isEmpty()) {
            return
        }
        for (i in 0 until poolSize) {
            ctx.supplyResult { session: Session ->
                logger.info("warmup session {}", session.id)

                queries.map { query -> session.prepareDataQuery(query, prepareSettings()) }
                    .reduceOrNull { u, v ->
                        u.thenCombine(v) { _, b -> b }
                    }
                    ?.whenComplete { _, _ ->
                        logger.info("warmup session {} completed", session.id)
                    }
            }.join()
        }
    }

    private fun prepareSettings(): PrepareDataQuerySettings {
        return PrepareDataQuerySettings().keepInQueryCache()
    }

    fun keepInQueryCache(): ExecuteDataQuerySettings {
        return ExecuteDataQuerySettings().keepInQueryCache()
    }

    fun <R> executeAsync(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<R>>>
    ): CompletableFuture<R> {
        val listeningQuery = ListeningQuery(queryInfo, query)
        return ctx.supplyResult { session -> listeningQuery.apply(session) }
            .thenApply { it.expect("error $queryInfo") }
    }

    @JvmOverloads
    fun <R> execute(
        queryInfo: String,
        timeout: Duration = defaultTimeout,
        query: Function<Session, CompletableFuture<Result<R>>>
    ): R {
        return executeAsync(queryInfo, query)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    @JvmOverloads
    fun <R> readFirstResultSet(
        queryInfo: String,
        timeout: Duration = defaultTimeout,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>
    ): List<R> {
        return readFirstResultSetAsync(queryInfo, query, mapper)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    fun <R> readFirstResultSetAsync(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>
    ): CompletableFuture<List<R>> {
        return executeAsync(queryInfo, query)
            .thenApply { result: DataQueryResult -> readFirstResultSet(result, mapper) }
    }

    fun <R> readFirstResultSet(result: DataQueryResult, mapper: Function<ResultSetReader, R>): List<R> {
        return if (result.isEmpty) {
            emptyList()
        } else readResultSetByIndex(result, 0, mapper)
    }

    fun <R> readResultSetByIndex(result: DataQueryResult, index: Int, mapper: Function<ResultSetReader, R>): List<R> {
        val resultList: MutableList<R> = ArrayList()
        val resultSet = result.getResultSet(index)
        while (resultSet.next()) {
            resultList.add(mapper.apply(resultSet))
        }
        return resultList.toList()
    }

    private inner class ListeningQuery<R>(
        private val queryInfo: String,
        private val query: Function<Session, CompletableFuture<Result<R>>>
    ) {
        private val retries = AtomicInteger(0)
        fun apply(session: Session): CompletableFuture<Result<R>> {
            val retryNumber = retries.getAndIncrement()
            if (retryNumber > 0) {
                listeners.forEach { listener -> listener.onRetry(queryInfo, retryNumber) }
            }
            listeners.forEach { listener -> listener.onQueryStarted(queryInfo) }

            val queryExecutionWatch = Stopwatch.createStarted()
            return query.apply(session)
                .whenComplete { r: Result<R>, ex: Throwable? ->
                    queryExecutionWatch.stop()

                    if (ex != null) {
                        listeners.forEach { listener: YDBQueryListener ->
                            listener.onQueryFailed(queryInfo, queryExecutionWatch.elapsed(), tryGetStatusFromEx(ex))
                        }
                    } else {
                        if (r.isSuccess) {
                            listeners.forEach { listener ->
                                listener.onQuerySuccess(queryInfo, queryExecutionWatch.elapsed())
                            }
                        } else {
                            val status = r.toStatus()
                            listeners.forEach { listener ->
                                listener.onQueryFailed(queryInfo, queryExecutionWatch.elapsed(), status)
                            }
                        }
                    }
                }
        }
    }
}

private fun tryGetStatusFromEx(t: Throwable): Status? {
    val cause = Async.unwrapCompletionException(t)
    return if (cause is UnexpectedResultException) {
        Status.of(cause.statusCode)
    } else {
        null
    }
}
