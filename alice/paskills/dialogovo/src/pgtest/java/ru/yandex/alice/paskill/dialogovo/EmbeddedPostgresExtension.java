package ru.yandex.alice.paskill.dialogovo;

import java.util.List;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.jupiter.api.extension.BeforeEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;
import org.springframework.jdbc.core.JdbcTemplate;

public class EmbeddedPostgresExtension implements BeforeEachCallback {

    private static final Logger logger = LogManager.getLogger();

    @Override
    public void beforeEach(ExtensionContext context) {
        JdbcTemplate jdbcTemplate = new JdbcTemplate(EmbeddedPostgresExecutionListener.createDataSource());
        List<String> tables = jdbcTemplate.queryForList("SELECT tablename FROM pg_tables WHERE schemaname = " +
                "current_schema()", String.class);

        logger.info("Tables: {}", tables);

        System.out.println("DELETING ALL TABLES");
        String allTables = tables.stream()
                // keep case if needed surrounding with commas
                .map(it -> !it.toLowerCase().equals(it) ? "\"" + it + "\"" : it)
                .collect(Collectors.joining(", "));
        jdbcTemplate.execute("TRUNCATE " + allTables);
    }
}
