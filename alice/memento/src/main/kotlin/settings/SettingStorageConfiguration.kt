package ru.yandex.alice.memento.settings

import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.memento.scanner.KeyMappingScanner
import ru.yandex.alice.memento.storage.StorageDao
import ru.yandex.monlib.metrics.registry.MetricRegistry

@Configuration
open class SettingStorageConfiguration {
    @Bean
    open fun settingsStorage(
        storageDao: StorageDao,
        scanner: KeyMappingScanner,
        metricRegistry: MetricRegistry,
    ): SettingsStorage {
        return SettingsStorageImpl(storageDao, scanner, metricRegistry)
    }
}
