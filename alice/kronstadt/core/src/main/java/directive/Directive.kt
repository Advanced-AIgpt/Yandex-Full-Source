package ru.yandex.alice.kronstadt.core.directive

@Target(AnnotationTarget.ANNOTATION_CLASS, AnnotationTarget.CLASS)
annotation class Directive(
    /**
     * Directive name
     */
    val value: String,
    /**
     * Mark directive as "event" so surface ignores MM answer on directive execution
     */
    val ignoreAnswer: Boolean = false
)
