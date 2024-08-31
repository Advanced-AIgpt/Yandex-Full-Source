package ru.yandex.alice.paskills.common.executor

import java.util.concurrent.TimeUnit

data class PoolConfig(
    val corePoolSize: Int,
    val maximumPoolSize: Int,
    val keepAliveTime: Long,
    val unit: TimeUnit,
)
