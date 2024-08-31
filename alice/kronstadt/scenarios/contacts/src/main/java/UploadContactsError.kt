package ru.yandex.alice.kronstadt.scenarios.contacts

class UploadContactsError(val uuid: String, val text: String)
    : Exception("Failed uploading contacts to messenger for uuid $uuid: $text")
