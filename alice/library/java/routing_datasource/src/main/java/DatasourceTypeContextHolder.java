package ru.yandex.alice.library.routingdatasource;


import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

class DatasourceTypeContextHolder {
    private static final ThreadLocal<DatasourceType> CONTEXT = new ThreadLocal<>();

    private DatasourceTypeContextHolder() {
        throw new UnsupportedOperationException();
    }

    public static void set(@Nonnull DatasourceType datasourceType) {
        CONTEXT.set(Objects.requireNonNull(datasourceType));
    }

    @Nullable
    public static DatasourceType getDatasourceType() {
        return CONTEXT.get();
    }

    public static void clear() {
        CONTEXT.remove();
    }
}
