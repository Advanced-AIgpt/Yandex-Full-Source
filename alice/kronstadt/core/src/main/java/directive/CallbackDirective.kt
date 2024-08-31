package ru.yandex.alice.kronstadt.core.directive

import com.fasterxml.jackson.annotation.JsonInclude

/**
 * Children has to be annotated with [Directive]
 */
@JsonInclude(JsonInclude.Include.NON_ABSENT)
interface CallbackDirective : MegaMindDirective
