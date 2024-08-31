package ru.yandex.quasar.billing;

import java.util.List;

import org.junit.jupiter.api.extension.BeforeEachCallback;
import org.junit.jupiter.api.extension.ExtensionContext;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.test.context.junit.jupiter.SpringExtension;
import org.springframework.transaction.annotation.Transactional;

@Transactional(readOnly = true)
public class EmbeddedPostgresExtension implements BeforeEachCallback {

    @Override
    @Transactional
    public void beforeEach(ExtensionContext context) {

        JdbcTemplate jdbcTemplate = SpringExtension.getApplicationContext(context).getBean(JdbcTemplate.class);
        List<String> tables = jdbcTemplate.queryForList("SELECT tablename FROM pg_tables WHERE schemaname = " +
                "current_schema()", String.class);

        System.out.println("DELETING ALL TABLES");
        if (!tables.isEmpty()) {
            try {

                jdbcTemplate.execute("TRUNCATE " + String.join(",", tables));

            } catch (Exception e) {
                throw new RuntimeException("Failed truncating tables: " + String.join(",", tables), e);
            }
        }
    }
}
