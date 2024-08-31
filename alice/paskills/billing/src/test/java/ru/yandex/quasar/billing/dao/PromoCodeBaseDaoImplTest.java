package ru.yandex.quasar.billing.dao;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.datasource.SimpleDriverDataSource;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.TransactionDefinition;
import org.springframework.transaction.support.DefaultTransactionDefinition;
import org.springframework.transaction.support.TransactionTemplate;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.PromoType;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.services.promo.Platform;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;


@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
public class PromoCodeBaseDaoImplTest {
    @Autowired
    private PromoCodeBaseDao promoCodeBaseDao;
    @Autowired
    private PlatformTransactionManager transactionManager;
    @Autowired
    private JdbcTemplate jdbcTemplate;
    private TransactionTemplate transactionTemplate;

    @BeforeEach
    public void setUp() throws Exception {
        // TransactionDefinition is needed to start new transaction when TransactionTemplate#execute is nested
        transactionTemplate = new TransactionTemplate(transactionManager,
                new DefaultTransactionDefinition(TransactionDefinition.PROPAGATION_REQUIRES_NEW));
    }

    @Test
    public void testConcurrentGetCode() {
        // given
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code1"));
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code2"));

        // then
        transactionTemplate.execute(status -> {
            PromoCodeBase code = promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus360,
                    Platform.YANDEXSTATION).get();
            PromoCodeBase code2 = transactionTemplate.execute(status2 ->
                    promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus360, Platform.YANDEXSTATION).get()
            );
            assertNotEquals(code.getCode(), code2.getCode());
            return 0;
        });
    }

    @Test
    public void testSequentialGetCodeWithExceptionInTransaction() {
        // given
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code1"));
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code2"));

        // when
        assertThrows(DummyException.class,
                () -> transactionTemplate.execute(status -> {
                    promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus360, Platform.YANDEXSTATION).get();
                    throw new DummyException();
                })
        );

        // then
        // first change was not saved due to error in a transaction
        PromoCodeBase code2 = transactionTemplate.execute(status ->
                promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus360, Platform.YANDEXSTATION).get()
        );
        assertEquals("code1", code2.getCode());
    }

    @Test
    public void testCodeWithPlatformSpecifiedIsUsed() {
        // given
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code1"));
        promoCodeBaseDao.save(PromoCodeBase.create("provider", PromoType.plus360, "code2", Platform.YANDEXSTATION));

        // when
        PromoCodeBase code = transactionTemplate.execute(status ->
                promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus360, Platform.YANDEXSTATION).get()
        );

        // then
        assertEquals("code2", code.getCode());
    }

    @Test
    public void testCodeWithPrototype() {

        List<String> tables = jdbcTemplate
                .queryForList("SELECT tablename FROM pg_tables WHERE schemaname = current_schema()",
                        String.class);

        if (tables.size() != 18) {
            var ds = (SimpleDriverDataSource) jdbcTemplate.getDataSource();

            throw new RuntimeException("No tables found after DB schema creation. found tables: " +
                    tables.size() + "\n" +
                    ds.getUrl() + "\n" +
                    ds.getUsername() + "\n" +
                    ds.getPassword() + "\n" +
                    ds.getSchema() + "\n");
        }
        // given
        jdbcTemplate.execute(
                "insert into promocode_prototype (promocode_prototype_id, promo_type, platform, code, task_id)\n" +
                        "values (1,'plus90','yandexmini','CODE1','TASK_ID')");
        promoCodeBaseDao.save(
                PromoCodeBase.create("provider", PromoType.plus90, "code2", Platform.YANDEXMINI, 1, "task1")
        );

        // when
        PromoCodeBase code = transactionTemplate.execute(status ->
                promoCodeBaseDao.queryNextUnusedCode("provider", PromoType.plus90, Platform.YANDEXMINI).get()
        );

        // then
        assertEquals(Integer.valueOf(1), code.getPrototypeId());
        assertEquals("task1", code.getTaskId());
    }


    private static class DummyException extends RuntimeException {
    }

}
