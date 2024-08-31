package ru.yandex.alice.paskill.dialogovo.ydb.listener

import com.yandex.ydb.core.Status
import java.time.Duration

interface YDBQueryListener {
    fun onQueryStarted(queryName: String)
    fun onRetry(queryName: String, retryNumber: Int)
    fun onQuerySuccess(queryName: String, duration: Duration)
    fun onQueryFailed(queryName: String, duration: Duration, status: Status?)
}
