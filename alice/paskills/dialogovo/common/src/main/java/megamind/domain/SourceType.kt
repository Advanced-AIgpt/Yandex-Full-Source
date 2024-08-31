package ru.yandex.alice.paskill.dialogovo.megamind.domain

enum class SourceType(
    val code: String,
    val isByUser: Boolean,
    val validationGroupClass: Class<out ValidationGroup>
) {
    USER("user", true, User::class.java),
    PING("ping", false, Ping::class.java),
    CONSOLE("console", true, Console::class.java),
    SYSTEM("system", false, System::class.java);

    interface ValidationGroup
    interface User : ValidationGroup
    interface Ping : ValidationGroup
    interface Console : ValidationGroup
    interface System : ValidationGroup
}
