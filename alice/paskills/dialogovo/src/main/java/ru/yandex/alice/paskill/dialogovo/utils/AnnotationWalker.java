package ru.yandex.alice.paskill.dialogovo.utils;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedParameterizedType;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import org.springframework.util.ConcurrentReferenceHashMap;

public class AnnotationWalker<TAnnotation extends Annotation> {
    private final Map<Class<?>, Field[]> declaredFieldsCache = new ConcurrentReferenceHashMap<>(256);
    private final Class<TAnnotation> annotation;

    public AnnotationWalker(Class<TAnnotation> annotation) {
        this.annotation = annotation;
    }

    public List<ValuePathWrapper<TAnnotation>> traverseObject(@Nullable Object obj) throws IllegalAccessException {
        var items = new ArrayList<ValuePathWrapper<TAnnotation>>();
        traverseObject(obj, items, "");
        return items;
    }

    private void traverseObject(@Nullable Object obj, List<ValuePathWrapper<TAnnotation>> items, String basePath)
            throws IllegalAccessException {
        if (obj == null) {
            return;
        }

        if (obj instanceof Iterable) {
            var i = 0;
            var path = basePath + "[";
            for (var item : (Iterable<?>) obj) {
                traverseObject(item, items, path + i++ + "].");
            }

            return;
        }

        for (var field : getCensoredFields(obj.getClass())) {
            var value = field.get(obj);
            if (value == null) {
                continue;
            }

            var fieldName = field.getName();
            if (String.class.equals(field.getType())) {
                var path = basePath + fieldName;
                items.add(new ValuePathWrapper<>(
                        path, (String) value, (s) -> field.set(obj, s),
                        field.getAnnotation(annotation)));

            } else if (value instanceof Optional) {
                var opt = ((Optional<?>) value);
                var path = basePath + fieldName;
                if (opt.isPresent()) {
                    var optValue = opt.get();
                    if (optValue instanceof String) {
                        var annotationInstance = field.getAnnotation(annotation);
                        if (annotationInstance == null) {
                            var annotatedType = field.getAnnotatedType();
                            if (annotatedType instanceof AnnotatedParameterizedType) {
                                var annotatedActualTypeArguments =
                                        ((AnnotatedParameterizedType) annotatedType).getAnnotatedActualTypeArguments();
                                if (annotatedActualTypeArguments.length > 0) {
                                    annotationInstance = annotatedActualTypeArguments[0].getAnnotation(annotation);
                                }
                            }
                        }

                        if (annotationInstance != null) {
                            items.add(new ValuePathWrapper<>(path,
                                    (String) optValue, (s) -> field.set(obj, Optional.of(s)), annotationInstance));
                        }
                    } else {
                        traverseObject(optValue, items, path + ".");
                    }
                }
            } else {
                traverseObject(value, items, basePath + fieldName + ".");
            }
        }
    }

    private Field[] getCensoredFields(Class<?> clazz) {
        var result = declaredFieldsCache.get(clazz);
        if (result == null) {
            result = Arrays.stream(clazz.getDeclaredFields()).filter(this::satisfies).toArray(Field[]::new);
            for (var field : result) {
                field.setAccessible(true);
            }

            declaredFieldsCache.put(clazz, result);
        }

        return result;
    }

    private boolean satisfies(Field x) {
        var type = x.getType();
        if (String.class.equals(type)) {
            return x.isAnnotationPresent(annotation);
        }

        if (Iterable.class.isAssignableFrom(type)) {
            return true;
        }

        if (Optional.class.isAssignableFrom(type)) {
            return true;
        }

        return type.isAnnotationPresent(annotation);
    }
}
