package ru.yandex.alice.paskill.dialogovo.utils;

import java.lang.annotation.Annotation;
import java.util.function.Function;

public class ValuePathWrapper<TAnnotation extends Annotation> {

    private final String path;

    private final String value;

    private final TAnnotation annotation;

    private final AnnotationWalkerApplier applier;

    public ValuePathWrapper(String path, String value, AnnotationWalkerApplier applier, TAnnotation annotation) {
        this.path = path;
        this.value = value;
        this.applier = applier;
        this.annotation = annotation;
    }

    public String getPath() {
        return path;
    }

    public String getValue() {
        return value;
    }

    public TAnnotation getAnnotation() {
        return annotation;
    }

    public void apply(String newValue) throws IllegalAccessException {
        this.applier.apply(newValue);
    }

    public void apply(Function<String, String> fn) throws IllegalAccessException {
        this.applier.apply(fn.apply(getValue()));
    }
}
