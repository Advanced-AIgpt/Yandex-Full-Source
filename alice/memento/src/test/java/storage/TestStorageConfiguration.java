package ru.yandex.alice.memento.storage;

import org.jetbrains.annotations.NotNull;
import org.springframework.boot.autoconfigure.condition.ConditionalOnMissingBean;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;

import ru.yandex.alice.paskills.common.ydb.YdbClient;


@TestConfiguration("settingsStorageDaoConfiguration")
public class TestStorageConfiguration extends SettingsStorageDaoConfiguration {

    public TestStorageConfiguration() {
        super();
    }

    @NotNull
    @Override
    @Bean
    @ConditionalOnProperty(name = "YDB_DATABASE")
    public StorageDao settingsStorageDao(YdbClient ydbClient) {
        return super.settingsStorageDao(ydbClient);
    }

    @Bean
    @ConditionalOnMissingBean(StorageDao.class)
    public StorageDao inMemoryStorage() {
        InMemoryStorageDao storageDao = new InMemoryStorageDao();
        return storageDao;
    }
}
