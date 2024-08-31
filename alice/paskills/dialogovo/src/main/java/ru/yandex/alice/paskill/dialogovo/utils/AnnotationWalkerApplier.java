package ru.yandex.alice.paskill.dialogovo.utils;

@FunctionalInterface
interface AnnotationWalkerApplier {
    void apply(String t) throws IllegalAccessException;
}
