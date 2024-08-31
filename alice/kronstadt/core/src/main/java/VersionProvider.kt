package ru.yandex.alice.kronstadt.core

interface VersionProvider {
    val version: String
    val branch: String
    val tag: String
}
