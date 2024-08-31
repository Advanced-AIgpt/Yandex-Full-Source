package ru.yandex.quasar.billing.services.processing.trust;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.web.client.RestTemplateBuilder;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.transaction.PlatformTransactionManager;

import ru.yandex.quasar.billing.RemoteServiceProxyExtension;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.SecretsConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.GenericProductDao;
import ru.yandex.quasar.billing.dao.SubscriptionProductsDAO;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;
import ru.yandex.quasar.billing.services.tvm.TestTvmClientImpl;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static ru.yandex.quasar.billing.RemoteServiceProxyMode.REPLAYING;

@SpringJUnitConfig(classes = {
        TestConfigProvider.class,
})
class TrustBillingServiceTest {

    @RegisterExtension
    static RemoteServiceProxyExtension stub = new RemoteServiceProxyExtension(
            "https://trust-payments-test.paysys.yandex.net:8028/trust-payments",
            REPLAYING
    );
    private final String uid = "999";
    private final String userIp = "127.0.0.1";
    private final RestTemplateBuilder builder = new RestTemplateBuilder();
    @Autowired
    private BillingConfig config;
    @MockBean
    private SubscriptionProductsDAO subscriptionProductsDAO;
    @MockBean
    private GenericProductDao genericProductDao;
    @MockBean
    private PlatformTransactionManager transactionManager;
    private TrustBillingService service;
    @Autowired
    private SecretsConfig secretsConfig;


    @BeforeEach
    void setUp() {
        // user real X-Service-Token to record
        var tvmClient = new TestTvmClientImpl(secretsConfig);
        var trustClient = new TrustBillingClientImpl(stub.getUrl(), builder, tvmClient, 10000, 10000,
                config.getTrustBillingConfig().getServiceToken(),
                config.getTrustBillingConfig().getTvmClientId(),
                false);

        service = new TrustBillingService(
                PaymentProcessor.TRUST,
                config,
                trustClient,
                genericProductDao,
                transactionManager
        );
    }

    @Test
    void startBinding() {

        NewBindingInfo actual = service.startBinding(uid, userIp, TrustCurrency.RUB, "http://return.url",
                TemplateTag.MOBILE);
        NewBindingInfo expected = new NewBindingInfo(
                "https://trust-test.yandex.ru/web/binding?purchase_token=285f909983dec1bce40509977afb40ff",
                "285f909983dec1bce40509977afb40ff");
        assertEquals(expected, actual);

        BindingInfo bindingInfo = service.getBindingInfo(uid, userIp, /*actual.getPurchaseToken()*/
                "285f909983dec1bce40509977afb40ff");

        assertEquals(new BindingInfo(BindingInfo.Status.wait_for_notification, null), bindingInfo);
    }

    @Test
    void testSuccessfulBingingInfo() {
        BindingInfo bindingInfo = service.getBindingInfo(uid, userIp, "0522f872c2a8c75a577b17808976849a");

        assertEquals(new BindingInfo(BindingInfo.Status.success, "card-x7062"), bindingInfo);
    }
}
