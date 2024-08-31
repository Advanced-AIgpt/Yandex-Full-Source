package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

public class ApphostRequestHandlingContext {
    private final String path;
    private final Map<String, Object> attributes = new LinkedHashMap<>();

    public ApphostRequestHandlingContext(String path) {
        this.path = Objects.requireNonNull(path);
    }

    public String getPath() {
        return path;
    }

    @Nullable
    public Object getAttribute(String name) {
        return attributes.get(name);
    }

    public Set<String> getAttributeNames() {
        return attributes.keySet();
    }

    public void setAttribute(String name, Object value) {
        attributes.put(Objects.requireNonNull(name), value);
    }

    public void removeAttribute(String name) {
        attributes.remove(name);
    }

}
