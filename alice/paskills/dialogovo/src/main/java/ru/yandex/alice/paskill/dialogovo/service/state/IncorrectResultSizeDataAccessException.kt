package ru.yandex.alice.paskill.dialogovo.service.state

/**
 * Similar to Spring Dao exception
 * [org.springframework.dao.IncorrectResultSizeDataAccessException]
 */
// todo: refactor parent exception.
internal class IncorrectResultSizeDataAccessException : RuntimeException()
