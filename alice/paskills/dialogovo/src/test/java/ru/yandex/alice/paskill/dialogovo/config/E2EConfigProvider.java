package ru.yandex.alice.paskill.dialogovo.config;

import org.springframework.boot.autoconfigure.jackson.JacksonAutoConfiguration;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Primary;

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.AppMetricaFirstUserEventDao;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.InMemoryAppMetricaFirstUserEventDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.logging.InMemorySkillRequestLogPersistent;
import ru.yandex.alice.paskill.dialogovo.service.logging.SkillRequestLogPersistent;
import ru.yandex.alice.paskill.dialogovo.service.purchase.InMemoryPurchaseCompleteResponseDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.purchase.PurchaseCompleteResponseDao;
import ru.yandex.alice.paskill.dialogovo.service.state.InMemorySkillStateDaoImpl;
import ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDao;

@TestConfiguration("configProvider")
@Import({JacksonAutoConfiguration.class, DialogovoJdbcConfiguration.class})
public class E2EConfigProvider extends TestConfigProvider {

    @Primary
    @Bean("skillStateDao")
    public SkillStateDao inMemorySkillStateDao() {
        return new InMemorySkillStateDaoImpl();
    }

    @Primary
    @Bean("inMemoryPurchaseCompleteResponseDao")
    public PurchaseCompleteResponseDao inMemoryPurchaseCompleteResponseDaoImpl() {
        return new InMemoryPurchaseCompleteResponseDaoImpl();
    }

    @Primary
    @Bean("inMemorySkillRequestLogPersistent")
    public SkillRequestLogPersistent inMemorySkillRequestLogPersistent() {
        return new InMemorySkillRequestLogPersistent();
    }


    @Primary
    @Bean("inMemoryAppMetricaFirstUserEventDao")
    public AppMetricaFirstUserEventDao inMemoryAppMetricaFirstUserEventDao() {
        return new InMemoryAppMetricaFirstUserEventDaoImpl();
    }
}
