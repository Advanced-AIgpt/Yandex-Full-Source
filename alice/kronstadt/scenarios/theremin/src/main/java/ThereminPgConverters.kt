package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import com.fasterxml.jackson.databind.ObjectMapper
import org.postgresql.util.PGobject
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.data.convert.ReadingConverter
import org.springframework.data.convert.WritingConverter
import ru.yandex.alice.kronstadt.core.db.PgConverter
import ru.yandex.alice.paskills.common.pgconverter.readPgObject
import ru.yandex.alice.paskills.common.pgconverter.toPgObject

@Configuration
open class ThereminPgConverters(
    private val objectMapper: ObjectMapper,
) {

    @Bean
    open fun backendSettingsToPgObjectConverter() = BackendSettingsToPgObjectConverter()

    @Bean
    open fun pgObjectToBackendSettingsConverter() = PgObjectToBackendSettingsConverter()

    @Bean
    open fun pgBackendSettingsToStringConverter() = PgBackendSettingsToStringConverter()

    @Bean
    open fun stringToBackendSettingsConverter() = StringToBackendSettingsConverter()

    @WritingConverter
    inner class BackendSettingsToPgObjectConverter :
        PgConverter<ThereminSkillInfoDB.BackendSettings, PGobject>(objectMapper) {
        override fun convert(source: ThereminSkillInfoDB.BackendSettings): PGobject {
            return objectMapper.toPgObject(source)
        }
    }

    @ReadingConverter
    inner class PgObjectToBackendSettingsConverter :
        PgConverter<PGobject, ThereminSkillInfoDB.BackendSettings>(objectMapper) {

        override fun convert(source: PGobject): ThereminSkillInfoDB.BackendSettings? {
            return objectMapper.readPgObject(source)
        }
    }

    @WritingConverter
    inner class PgBackendSettingsToStringConverter :
        PgConverter<ThereminSkillInfoDB.BackendSettings, String>(objectMapper) {

        override fun convert(source: ThereminSkillInfoDB.BackendSettings): String? {
            return jsonToString(source)
        }
    }

    @ReadingConverter
    inner class StringToBackendSettingsConverter :
        PgConverter<String, ThereminSkillInfoDB.BackendSettings>(objectMapper) {

        override fun convert(source: String): ThereminSkillInfoDB.BackendSettings? {
            return stringToJsonObject(source, ThereminSkillInfoDB.BackendSettings::class.java)
        }
    }
}
