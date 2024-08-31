package ru.yandex.alice.paskills.common.ydb

import com.google.common.base.Stopwatch
import com.yandex.ydb.core.Result
import com.yandex.ydb.core.Status
import com.yandex.ydb.core.StatusCode
import com.yandex.ydb.core.UnexpectedResultException
import com.yandex.ydb.core.utils.Async
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.SessionRetryContext
import com.yandex.ydb.table.query.DataQuery
import com.yandex.ydb.table.query.DataQueryResult
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.settings.ExecuteDataQuerySettings
import com.yandex.ydb.table.settings.PrepareDataQuerySettings
import com.yandex.ydb.table.transaction.TxControl
import org.apache.logging.log4j.LogManager
import ru.yandex.alice.paskills.common.ydb.listener.YDBQueryListener
import java.time.Duration
import java.util.concurrent.CancellationException
import java.util.concurrent.CompletableFuture
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.function.Function

class YdbClientImpl(
    private val ctx: SessionRetryContext,
    private val listeners: List<YDBQueryListener>,
    private val poolSize: Int,
    private val clientTimeout: Duration,
    private val operationTimeout: Duration = clientTimeout.minus(Duration.ofMillis(50)),
    private val skipWarmUp: Boolean = false,
) : YdbClient {

    init {
        if (clientTimeout < Duration.ofMillis(50)) {
            throw RuntimeException("client timeout must be greater then 50ms")
        }
    }

    private val defaultTimeout = Duration.ofSeconds(5)

    /**
     * Prepare queries in session. This establishes connections warming them up and putting prepared query into cache
     * of at least some of session
     *
     * @param queries list of queries to prepare
     */
    override fun warmUpSessionPool(queries: List<String>) {
        if (queries.isEmpty()) {
            return
        }
        if (!skipWarmUp) {
            // initialize large YDB class used by the client
            val executor = Executors.newCachedThreadPool()

            val s = Stopwatch.createStarted()
            logger.info("Getting first session")
            ctx.supplyStatus { CompletableFuture.completedFuture(Status.of(StatusCode.SUCCESS)) }.join()
            logger.info("First session obtained in ${s.elapsed(TimeUnit.MILLISECONDS)}ms")

            val sessions = initializeSessions(executor)
            val ids = prepareStatements(queries, executor)

            logger.info("All session from pool warmed up = ${sessions.size == ids.size}")
            executor.shutdown()
        } else {
            logger.info("Skipping warmup")
        }
    }

    private fun prepareStatements(
        queries: List<String>,
        executor: ExecutorService?
    ): ConcurrentHashMap<String, Boolean> {
        logger.info("Preparing statements in session pool sessions")
        val latch = CountDownLatch(poolSize)
        val ids = ConcurrentHashMap<String, Boolean>(poolSize)
        val failedIds = ConcurrentHashMap<String, Boolean>(poolSize)
        IntRange(1, poolSize).map { poolIndex ->
            ctx.supplyResult { session: Session ->
                latch.countDown()
                val sw = Stopwatch.createStarted()

                queries.fold(CompletableFuture.completedFuture<Result<DataQuery>>(null)) { acc, query ->
                    acc.thenCompose {
                        session.prepareDataQuery(
                            query,
                            prepareSettings(clientTimeout = Duration.ofSeconds(5))
                        )
                    }
                }
                    .whenComplete { result, e: Throwable? ->
                        val elapsed = sw.stop().elapsed(TimeUnit.MILLISECONDS)
                        if (e == null && result.isSuccess) {
                            ids[poolIndex.toString()] = true
                            logger.debug("warmup session ${session.id} completed in ${elapsed}ms")
                        } else {
                            failedIds[poolIndex.toString()] = true
                            if (e == null) {
                                if (result?.isSuccess == false) {
                                    logger.warn("session ${session.id} preparation failure: ${result.issues.map { issue -> issue.message }}")
                                }
                            } else {
                                logger.error("failed to warmup session ${session.id}", e)
                            }
                        }
                    }
                    .thenCombine(CompletableFuture.runAsync({
                        val stopwatch = Stopwatch.createStarted()
                        latch.await(30, TimeUnit.SECONDS)
                        val duration = stopwatch.stop().elapsed(TimeUnit.MICROSECONDS)
                        logger.info("Session: ${session.id}. Waited for latch: ${duration} microseconds")
                    }, executor)) { u, _ -> u }
            }.whenComplete { _, e: Throwable? -> if (e != null) failedIds[poolIndex.toString()] = true }
        }
            .fold(CompletableFuture.completedFuture(Result.success<DataQuery>(null))) { u, v ->
                u.thenCombine(v, { _, b -> b })
            }
            .join()
        logger.info("Warmed up ${ids.size} session, failed: ${failedIds.size} pool size: ${poolSize}")
        return ids
    }

    private fun initializeSessions(executor: ExecutorService): ConcurrentHashMap<String, Boolean> {
        val latch = CountDownLatch(poolSize)
        val sessions = ConcurrentHashMap<String, Boolean>(poolSize)

        logger.info("Initialize sessions in session pool")
        IntRange(1, poolSize).asSequence()
            .map {
                ctx.supplyStatus { session ->
                    sessions[session.id] = true
                    latch.countDown()
                    CompletableFuture.completedFuture(Status.of(StatusCode.SUCCESS))
                        .thenCombine(CompletableFuture.runAsync(
                            {
                                val stopwatch = Stopwatch.createStarted()
                                logger.debug("Session: ${session.id}. Waiting for latch")
                                latch.await(15, TimeUnit.SECONDS)
                                val duration = stopwatch.stop().elapsed(TimeUnit.MICROSECONDS)
                                logger.info("Session: ${session.id}. Waited for latch: ${duration} microseconds")
                            },
                            executor
                        ), { u, _ -> u })
                }
            }
            .fold(CompletableFuture.completedFuture(Status.SUCCESS)) { u, v -> u.thenCombine(v) { _, b -> b } }
            .join()

        logger.info("Session pool initialized: ${sessions.size}")
        return sessions
    }

    private fun prepareSettings(clientTimeout: Duration = this.clientTimeout): PrepareDataQuerySettings {
        return PrepareDataQuerySettings()
            .keepInQueryCache()
            .setOperationTimeout(operationTimeout)
            .setTimeout(clientTimeout)
    }

    override fun keepInQueryCache(): ExecuteDataQuerySettings {
        return ExecuteDataQuerySettings()
            .keepInQueryCache()
            .setOperationTimeout(operationTimeout)
            .setTimeout(clientTimeout)
    }

    override fun <R> executeAsync(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<R>>>
    ): CompletableFuture<R> {
        val listeningQuery = ListeningQuery(queryInfo, query)
        return ctx.supplyResult { session: Session -> listeningQuery.apply(session) }
            .thenApply {
                if (it.code == StatusCode.CLIENT_CANCELLED) {
                    throw CancellationException("client cancelled operation: ${queryInfo}")
                } else {
                    it.expect("error $queryInfo")
                }
            }
    }

    override fun <R> execute(
        queryInfo: String,
        timeout: Duration,
        query: Function<Session, CompletableFuture<Result<R>>>
    ): R {
        return executeAsync(queryInfo, query)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    override fun executeRwAsync(queryInfo: String, query: String, params: Params): CompletableFuture<DataQueryResult> {
        return executeAsync(queryInfo) { session: Session ->
            session.executeDataQuery(query, TxControl.serializableRw(), params, keepInQueryCache())
        }
    }

    override fun executeRw(queryInfo: String, query: String, params: Params, timeout: Duration): DataQueryResult {
        return executeRwAsync(queryInfo, query, params)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    override fun executeQueryRo(queryInfo: String, query: String, params: Params, timeout: Duration): DataQueryResult {
        return executeQueryRoAsync(queryInfo, query, params)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    override fun executeQueryRoAsync(
        queryInfo: String,
        query: String,
        params: Params
    ): CompletableFuture<DataQueryResult> {
        return executeAsync(queryInfo) { session: Session ->
            session.executeDataQuery(query, TxControl.onlineRo(), params, keepInQueryCache())
        }
    }

    override fun <R> readFirstResultSetAsync(
        queryInfo: String,
        query: String,
        params: Params,
        mapper: Function<ResultSetReader, R>,
    ): CompletableFuture<List<R>> {
        return executeAsync(queryInfo) { session ->
            session.executeDataQuery(query, TxControl.onlineRo(), params, keepInQueryCache())
        }.thenApply { result ->
            readFirstResultSet(result, mapper)
        }
    }

    override fun <R> execute(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<R>>>,
    ): R {
        return execute(queryInfo, defaultTimeout, query)
    }

    override fun <R> readFirstResultSet(
        queryInfo: String,
        timeout: Duration,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>,
    ): List<R> {
        return readFirstResultSetAsync(queryInfo, query, mapper)
            .orTimeout(timeout.toMillis(), TimeUnit.MILLISECONDS)
            .join()
    }

    override fun <R> readFirstResultSet(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>
    ): List<R> {
        return readFirstResultSet(queryInfo, defaultTimeout, query, mapper)
    }

    override fun <R> readFirstResultSetAsync(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>
    ): CompletableFuture<List<R>> {
        return executeAsync(queryInfo, query)
            .thenApply { result: DataQueryResult -> readFirstResultSet(result, mapper) }
    }

    override fun <R> readFirstResultSet(result: DataQueryResult, mapper: Function<ResultSetReader, R>): List<R> {
        return if (result.isEmpty) {
            emptyList()
        } else readResultSetByIndex(result, 0, mapper)
    }

    override fun <R> readResultSetByIndex(
        result: DataQueryResult,
        index: Int,
        mapper: Function<ResultSetReader, R>
    ): List<R> {
        val resultList: MutableList<R> = ArrayList()
        val resultSet = result.getResultSet(index)
        while (resultSet.next()) {
            resultList.add(mapper.apply(resultSet))
        }
        return resultList
    }

    private inner class ListeningQuery<R>(
        private val queryInfo: String,
        private val query: Function<Session, CompletableFuture<Result<R>>>
    ) {
        private val retries = AtomicInteger(0)
        private val sw = Stopwatch.createStarted()

        fun apply(session: Session): CompletableFuture<Result<R>> {
            val retryNumber = retries.getAndIncrement()
            if (retryNumber > 0) {
                listeners.forEach { it.onRetry(queryInfo, retryNumber) }
            }
            listeners.forEach { it.onQueryStarted(queryInfo, sw.elapsed()) }
            val queryExecutionWatch = Stopwatch.createStarted()
            return query.apply(session)
                .whenComplete { r: Result<R>?, ex: Throwable? ->
                    queryExecutionWatch.stop()
                    if (ex != null) {
                        listeners.forEach {
                            it.onQueryFailed(queryInfo, queryExecutionWatch.elapsed(), tryGetStatusFromEx(ex))
                        }
                    } else {
                        if (r?.isSuccess == true) {
                            listeners.forEach {
                                it.onQuerySuccess(queryInfo, queryExecutionWatch.elapsed())
                            }
                        } else {
                            listeners.forEach {
                                it.onQueryFailed(
                                    queryInfo,
                                    queryExecutionWatch.elapsed(), r?.toStatus() ?: Status.of(StatusCode.UNDETERMINED)
                                )
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
    }

    companion object {
        private val logger = LogManager.getLogger()
    }
}
