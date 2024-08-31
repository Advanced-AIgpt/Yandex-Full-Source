package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

import org.postgresql.util.PGobject
import org.springframework.context.annotation.Bean
import org.springframework.context.annotation.Configuration
import org.springframework.context.annotation.Primary
import org.springframework.core.convert.converter.Converter
import org.springframework.data.jdbc.core.convert.JdbcCustomConversions
import org.springframework.data.jdbc.repository.config.AbstractJdbcConfiguration
import org.springframework.data.relational.core.dialect.Dialect
import org.springframework.data.relational.core.dialect.PostgresDialect
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcOperations

@Configuration
open class PaskillsJdbcConfiguration(
    private val toPgObjectConverters: List<Converter<*, PGobject>>,
    private val fromPgObjectConverters: List<Converter<PGobject, *>>,
    private val fromStringConverters: List<Converter<String, *>>,
    private val toStringConverters: List<Converter<*, String>>,
) : AbstractJdbcConfiguration() {

    @Bean
    override fun jdbcDialect(
        operations: NamedParameterJdbcOperations
    ): Dialect = PostgresDialect.INSTANCE

    @Primary
    @Bean
    override fun jdbcCustomConversions() = JdbcCustomConversions(
        toPgObjectConverters +
            fromPgObjectConverters +
            fromStringConverters +
            toStringConverters
    )
}
