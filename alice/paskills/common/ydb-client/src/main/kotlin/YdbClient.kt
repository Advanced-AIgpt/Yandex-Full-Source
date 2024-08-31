package ru.yandex.alice.paskills.common.ydb

import com.yandex.ydb.core.Result
import com.yandex.ydb.table.Session
import com.yandex.ydb.table.query.DataQueryResult
import com.yandex.ydb.table.query.Params
import com.yandex.ydb.table.result.ResultSetReader
import com.yandex.ydb.table.settings.ExecuteDataQuerySettings
import java.time.Duration
import java.util.concurrent.CompletableFuture
import java.util.function.Function

interface YdbClient {
    fun warmUpSessionPool(queries: List<String>)
    fun keepInQueryCache(): ExecuteDataQuerySettings
    fun <R> executeAsync(queryInfo: String, query: Function<Session, CompletableFuture<Result<R>>>): CompletableFuture<R>
    fun <R> execute(queryInfo: String, timeout: Duration, query: Function<Session, CompletableFuture<Result<R>>>): R
    fun executeRwAsync(queryInfo: String, query: String, params: Params): CompletableFuture<DataQueryResult>
    fun executeRw(queryInfo: String, query: String, params: Params, timeout: Duration): DataQueryResult
    fun executeQueryRo(queryInfo: String, query: String, params: Params, timeout: Duration): DataQueryResult
    fun executeQueryRoAsync(queryInfo: String, query: String, params: Params): CompletableFuture<DataQueryResult>
    fun <R> readFirstResultSetAsync(
        queryInfo: String,
        query: String,
        params: Params,
        mapper: Function<ResultSetReader, R>,
    ): CompletableFuture<List<R>>

    fun <R> execute(queryInfo: String, query: Function<Session, CompletableFuture<Result<R>>>): R
    fun <R> readFirstResultSet(
        queryInfo: String,
        timeout: Duration,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>,
    ): List<R>

    fun <R> readFirstResultSet(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>,
    ): List<R>

    fun <R> readFirstResultSetAsync(
        queryInfo: String,
        query: Function<Session, CompletableFuture<Result<DataQueryResult>>>,
        mapper: Function<ResultSetReader, R>,
    ): CompletableFuture<List<R>>

    fun <R> readFirstResultSet(result: DataQueryResult, mapper: Function<ResultSetReader, R>): List<R>
    fun <R> readResultSetByIndex(result: DataQueryResult, index: Int, mapper: Function<ResultSetReader, R>): List<R>
}
