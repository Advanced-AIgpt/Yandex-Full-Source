package ru.yandex.alice.library.routingdatasource;

import java.util.Map;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.sql.DataSource;

import org.springframework.jdbc.datasource.lookup.AbstractRoutingDataSource;

public class RoutingDataSource extends AbstractRoutingDataSource implements AutoCloseable {

    private final DataSource replicaDataSource;
    private final DataSource primaryDataSource;

    public RoutingDataSource(@Nonnull DataSource primary, @Nonnull DataSource replica) {
        super();
        this.primaryDataSource = Objects.requireNonNull(primary);
        this.replicaDataSource = Objects.requireNonNull(replica);
        setTargetDataSources(Map.of(DatasourceType.PRIMARY, this.primaryDataSource, DatasourceType.REPLICA,
                this.replicaDataSource));
        setDefaultTargetDataSource(this.primaryDataSource);
    }

    public DataSource getPrimaryDataSource() {
        return primaryDataSource;
    }

    public DataSource getReplicaDataSource() {
        return replicaDataSource;
    }

    @Override
    protected Object determineCurrentLookupKey() {
        return Objects.requireNonNullElse(DatasourceTypeContextHolder.getDatasourceType(), DatasourceType.PRIMARY);
    }

    @Override
    public void close() throws Exception {
        Exception ex = null;
        if (replicaDataSource instanceof AutoCloseable) {
            try {
                ((AutoCloseable) replicaDataSource).close();
            } catch (Exception e) {
                ex = e;
            }
        }
        if (primaryDataSource instanceof AutoCloseable) {
            try {
                ((AutoCloseable) primaryDataSource).close();
            } catch (Exception e) {
                ex = e;
            }
        }
        if (ex != null) {
            throw ex;
        }
    }
}
