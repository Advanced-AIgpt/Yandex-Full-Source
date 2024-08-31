package ru.yandex.alice.kronstadt.scenarios.alice4business.providers

import org.springframework.data.jdbc.repository.query.Query
import org.springframework.data.repository.Repository
import org.springframework.data.repository.query.Param
import org.springframework.transaction.annotation.Transactional
import ru.yandex.alice.kronstadt.scenarios.alice4business.model.ActivationCode
import java.sql.Timestamp
import java.util.UUID

@Transactional(readOnly = false, transactionManager = "alice4BusinessTransactionManager")
interface ActivationCodeDao : Repository<ActivationCode, String> {
    @Query(
        """
            SELECT code, device_id, created_at
            FROM activation_codes
            WHERE device_id=:device_id and
                  created_at>=:created_at
            ORDER BY created_at DESC LIMIT 1
        """
    )
    fun getByDeviceIdCreatedLaterThan(
        @Param("device_id") deviceId: UUID,
        @Param("created_at") createdAt: Timestamp
    ): ActivationCode?

    @Query(
        """
            SELECT EXISTS (
                SELECT 1
                FROM activation_codes
                WHERE code=:code
            )
        """
    )
    fun existsByCode(@Param("code") code: String): Boolean

    @Query(
        """
            INSERT INTO activation_codes(code, device_id, created_at)
            VALUES (:code, :device_id, :created_at)
            RETURNING *
        """
    )
    fun save(
        @Param("code") code: String,
        @Param("device_id") deviceId: UUID,
        @Param("created_at") createdAt: Timestamp
    ): ActivationCode
}
