package ru.yandex.alice.kronstadt.scenarios.alice4business.providers

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.data.repository.query.Param
import org.springframework.transaction.annotation.Transactional
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.BusinessDeviceWithStatus

@Transactional(
    readOnly = true,
    transactionManager = "alice4BusinessTransactionManager"
)
interface BusinessDeviceDao : Repository<BusinessDeviceWithStatus, String> {
    @Query(
        """
            SELECT id, platform, status
            FROM devices
            WHERE device_id=:device_id and
                  platform=:platform and
                  deleted_at IS NULL
        """
    )
    fun get(@Param("device_id") deviceId: String, @Param("platform") platform: String): BusinessDeviceWithStatus?
}
