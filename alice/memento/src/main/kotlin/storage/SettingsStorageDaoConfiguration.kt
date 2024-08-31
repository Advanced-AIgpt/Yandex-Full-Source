package ru.yandex.alice.memento.storage

import org.apache.logging.log4j.LogManager
import org.springframework.beans.factory.annotation.Autowired
import org.springframework.beans.factory.annotation.Value
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import ru.yandex.alice.memento.proto.MementoApiProto
import ru.yandex.alice.memento.scanner.KeyMappingScanner
import ru.yandex.alice.paskills.common.ydb.YdbClient

@Configuration(proxyBeanMethods = false)
internal open class SettingsStorageDaoConfiguration() {
    private val logger = LogManager.getLogger()

    @Value("\${ydb.operationTimeout}")
    private val timeout: Long = 0

    @Value("\${ydb.warmup.waveCount}")
    private val warmupWaveCount: Int = 0

    @Value("\${ydb.warmup.queriesInWave}")
    private val queriesInWave: Int = 0

    @Autowired
    private lateinit var scanner: KeyMappingScanner

    @Bean
    open fun settingsStorageDao(ydbClient: YdbClient): StorageDao {
        val storageDao = StorageDaoImpl(ydbClient, timeout)
        storageDao.warmup(queriesInWave, warmupWaveCount)
        warmupWithData(storageDao)
        return storageDao
    }

    private fun warmupWithData(storageDao: StorageDaoImpl) {
        logger.info("Perform warmup with data")
        val userSettings = MementoApiProto.EConfigKey.values()
            .filter {
                it != MementoApiProto.EConfigKey.UNRECOGNIZED
                    && it != MementoApiProto.EConfigKey.CK_UNDEFINED
                    && scanner.getDefaultForKey(it) != null
            }
            .associate { scanner.getDbKey(it) to scanner.getDefaultForKey(it) }

        val surfaceSettings = MementoApiProto.EDeviceConfigKey.values()
            .filter {
                it != MementoApiProto.EDeviceConfigKey.UNRECOGNIZED
                    && it != MementoApiProto.EDeviceConfigKey.DCK_UNDEFINED
                    && scanner.getDefaultForKey(it) != null
            }
            .associate { scanner.getDbKey(it) to scanner.getDefaultForKey(it) }

        val userId = "-1"
        val surfaceId = "tmp_surf"
        val storedData = StoredData(
            userSettings = userSettings,
            deviceSettings = mapOf(surfaceId to surfaceSettings),
            scenariosData = mapOf(),
            surfaceScenariosData = mapOf(),
        )
        storageDao.update(userId, storedData, false)
        storageDao.fetchAll(userId, setOf(surfaceId), false)
        storageDao.update(surfaceId, storedData, true)
        storageDao.fetchAll(surfaceId, setOf(surfaceId), true)

        logger.info("Perform warmup with data finished")
    }
}
