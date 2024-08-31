package ru.yandex.alice.paskill.dialogovo.domain

import ru.yandex.alice.kronstadt.core.directive.OpenUriDirective
import ru.yandex.alice.kronstadt.core.directive.TypeTextDirective
import ru.yandex.alice.kronstadt.core.layout.Button
import java.net.URI
import java.time.Duration

class StartAccountLinkingButton(text: String, directives: OpenUriDirective, val authPageUrl: URI) :
    Button(text = text, directives = listOf(directives))

class StartPurchaseButton(text: String, directive: OpenUriDirective, val offerUrl: URI, val offerUuid: String) :
    Button(text = text, directives = listOf(directive))

class RequestGeolocationButton(text: String, val period: Duration) :
    Button(text = text, directives = listOf(TypeTextDirective(text))) {

    override fun toString(): String {
        return "RequestGeolocationButton(text='$text', period=$period, directives=$directives)"
    }
}
