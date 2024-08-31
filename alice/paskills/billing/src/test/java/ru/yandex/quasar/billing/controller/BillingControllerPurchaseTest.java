package ru.yandex.quasar.billing.controller;

import java.math.BigDecimal;
import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.servlet.http.HttpServletRequest;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.hamcrest.Matchers;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.stubbing.Answer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.jdbc.DataSourceTransactionManagerAutoConfiguration;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.context.annotation.Bean;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.web.client.RestTemplate;

import ru.yandex.quasar.billing.beans.ContentMetaInfo;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.PricingOptionType;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.config.TrustBillingConfig;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.SubscriptionProductsDAO;
import ru.yandex.quasar.billing.dao.UserPastesDAO;
import ru.yandex.quasar.billing.dao.UserPromoCodeDao;
import ru.yandex.quasar.billing.dao.UserPurchaseLockDAO;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSkillProductDao;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.exception.RetryPurchaseException;
import ru.yandex.quasar.billing.providers.AvailabilityInfo;
import ru.yandex.quasar.billing.providers.IContentProvider;
import ru.yandex.quasar.billing.providers.PromoCodeActivationException;
import ru.yandex.quasar.billing.providers.ProviderActiveSubscriptionInfo;
import ru.yandex.quasar.billing.providers.ProviderManager;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.ProviderPromoCodeActivationResult;
import ru.yandex.quasar.billing.providers.ProviderPurchaseException;
import ru.yandex.quasar.billing.providers.RejectionReason;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.BillingServiceImpl;
import ru.yandex.quasar.billing.services.ProviderPurchaseServiceImpl;
import ru.yandex.quasar.billing.services.TakeoutService;
import ru.yandex.quasar.billing.services.UnistatService;
import ru.yandex.quasar.billing.services.UserPurchaseLockService;
import ru.yandex.quasar.billing.services.content.ContentService;
import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient;
import ru.yandex.quasar.billing.services.processing.ProcessingServicesConfig;
import ru.yandex.quasar.billing.services.processing.trust.PaymentMethodsClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingClient;
import ru.yandex.quasar.billing.services.processing.trust.TrustPaymentShortInfo;
import ru.yandex.quasar.billing.services.processing.yapay.YandexPayClient;
import ru.yandex.quasar.billing.services.promo.QuasarBackendClient;
import ru.yandex.quasar.billing.services.promo.QuasarPromoService;
import ru.yandex.quasar.billing.services.skills.SkillsService;
import ru.yandex.quasar.billing.services.sup.MobilePushService;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static com.google.common.collect.Lists.newArrayList;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.services.processing.TrustCurrency.RUB;

@SpringJUnitConfig(classes = {BillingController.class,
        AuthorizationContext.class,
        BillingControllerPurchaseTest.TestConfig.class,
        UserPurchaseLockService.class,
        TestConfigProvider.class,
        ProviderManager.class,
        ProviderPurchaseServiceImpl.class,
        BillingServiceImpl.class,
        ContentService.class,
        TestConfigProvider.class,
        DataSourceTransactionManagerAutoConfiguration.class,
        ProcessingServicesConfig.class,
        RestTemplate.class,
        ObjectMapper.class,
})
@MockBean(name = "yandexPayTrustClient", classes = TrustBillingClient.class)
@MockBean(name = "mediaBillingTrustClient", classes = PaymentMethodsClient.class)
@MockBean(classes = {
        UserPastesDAO.class,
        UserPromoCodeDao.class,
        MobilePushService.class,
        SubscriptionProductsDAO.class,
        UnistatService.class,
        QuasarPromoService.class,
        SkillsService.class,
        TakeoutService.class,
        TvmHeaders.class,
        MediaBillingClient.class,
        YandexPayClient.class,
        UserSkillProductDao.class,
        QuasarBackendClient.class,
})
class BillingControllerPurchaseTest implements PricingOptionTestUtil {
    private static final String UID = "999";
    private static final String CARD_NUMBER = "card-x1111";
    private static int maxRetries;
    private final String purchaseToken = "token";
    private final String providerToken = "provider_token";
    private final String ip = "ip";
    private final Long subscriptionId = 1L;
    @MockBean
    private UserPurchasesDAO userPurchasesDAO;
    @MockBean
    private AuthorizationService authorizationService;
    @MockBean
    private UserSubscriptionsDAO userSubscriptionsDAO;
    @MockBean(name = "trustBillingClient")
    private TrustBillingClient trustBillingClient;
    @MockBean
    private UserPurchaseLockDAO userPurchaseLockDAO;
    @SpyBean
    private IContentProvider contentProvider;
    @Autowired
    private BillingController billingController;
    @Autowired
    private BillingConfig config;
    private ProviderContentItem purchasingItem;
    private PricingOption selectedPricingOption;
    private PurchaseInfo purchaseInfo;
    private HttpServletRequest request = mock(HttpServletRequest.class);
    private SubscriptionInfo subscriptionInfo;

    @BeforeEach
    void setUp() {
        maxRetries = config.getTrustBillingConfig().getMaxRetriesCount();
        when(userPurchaseLockDAO.acquireLock(any(), any())).thenReturn(true);
        purchasingItem = ProviderContentItem.create(ContentType.MOVIE, "film_id");
        selectedPricingOption = createPricingOption("test", new BigDecimal(100), purchasingItem);
        purchaseInfo = PurchaseInfo.createSinglePayment(-1L, Long.valueOf(UID), purchaseToken, purchasingItem,
                selectedPricingOption, PurchaseInfo.Status.STARTED, null, 1L, null,
                selectedPricingOption.getProvider(), null, PaymentProcessor.TRUST, null, null);

        when(userPurchasesDAO.getPurchaseInfo(-1L)).thenReturn(Optional.of(purchaseInfo));
        when(userPurchasesDAO.getPurchaseInfo(Long.valueOf(UID), purchaseToken)).thenReturn(purchaseInfo);

        when(userPurchasesDAO.getPurchaseRetries(-1L)).thenAnswer(a -> purchaseInfo.getRetriesCount());
        when(contentProvider.getPricingOptions(eq(purchasingItem), any()))
                .thenReturn(ProviderPricingOptions.create(false, Collections.singletonList(selectedPricingOption),
                        null));

        doAnswer((Answer<Void>) invocation -> {
            purchaseInfo.setRetriesCount(purchaseInfo.getRetriesCount() + 1);
            return null;
        }).when(userPurchasesDAO).incrementPurchaseRetries(eq(-1L));
        when(authorizationService.getProviderTokenByUid(eq(UID), eq("test")))
                .thenReturn(Optional.of(providerToken));

        when(request.getRemoteAddr()).thenReturn(ip);
        when(authorizationService.getUserIp(any())).thenReturn(ip);

        when(userSubscriptionsDAO.getSubscriptionInfo(subscriptionId)).thenAnswer(it -> subscriptionInfo);
    }

    @Test
    void testOnBillingHoldApplied() throws ProviderPurchaseException {
        when(trustBillingClient.getPaymentShortInfo(eq(UID), any(), eq(purchaseToken))).thenReturn(authorizedPayment());

        doThrow(new ProviderPurchaseException(PurchaseInfo.Status.ERROR_TRY_LATER))
                .when(contentProvider).processPurchase(any(), eq(selectedPricingOption), anyString(), anyString());
        doReturn(ProviderPricingOptions.create(false, newArrayList(selectedPricingOption), null))
                .when(contentProvider).getPricingOptions(eq(purchasingItem), anyString());

        // execute call twice. On the first execution exception is raised as we have MAX_RETRIES=1
        assertThat(maxRetries, Matchers.greaterThan(0));

        for (int i = 0; i < maxRetries; i++) {
            try {
                billingController.onBillingHoldApplied("-1", null, request);
                fail("RetryPurchaseException not raised");
            } catch (RetryPurchaseException ignored) {

            }
        }

        // on forth attempt no exception is raised as we stop trying.
        assertEquals("CANCELLED", billingController.onBillingHoldApplied("-1", null, request).getResult());
        assertEquals(maxRetries, (int) purchaseInfo.getRetriesCount());

    }

    @Test
    void testOnTrustRefund() {
        when(userPurchasesDAO.getPurchaseInfo(purchaseToken)).thenReturn(Optional.of(purchaseInfo));

        SimpleResult simpleResult = billingController.onTrustRefund(purchaseToken);

        assertEquals("PURCHASE_REFUNDED", simpleResult.getResult());
        verify(userPurchasesDAO).setRefunded(purchaseInfo.getPurchaseId());
    }

    @Test
    void testOnTrustRefundNotFound() {
        when(userPurchasesDAO.getPurchaseInfo(purchaseToken)).thenReturn(Optional.empty());

        SimpleResult simpleResult = billingController.onTrustRefund(purchaseToken);

        assertEquals("PURCHASE_NOT_FOUND", simpleResult.getResult());
    }

    @Nonnull
    private TrustPaymentShortInfo authorizedPayment() {
        return new TrustPaymentShortInfo(null, null, "authorized", CARD_NUMBER, "type", BigDecimal.TEN, RUB,
                Instant.now(), CARD_NUMBER, null, List.of(new TrustPaymentShortInfo.TrustOrderInfo("orderId",
                BigDecimal.TEN)));
    }


    @TestConfiguration
    static class TestConfig implements PricingOptionTestUtil {

        @Bean
        BillingConfig billingConfig() {
            BillingConfig config = new BillingConfig();
            config.setTrustBillingConfig(new TrustBillingConfig());
            config.getTrustBillingConfig().setMaxRetriesCount(maxRetries);
            return config;
        }

        @Bean
        IContentProvider contentProvider() {
            return new IContentProvider() {
                @Override
                public String getProviderName() {
                    return "test";
                }

                @Override
                public String getSocialAPIServiceName() {
                    return "test";
                }

                @Nullable
                @Override
                public String getSocialAPIClientId() {
                    return null;
                }

                @Override
                public ContentMetaInfo getContentMetaInfo(ProviderContentItem item) {
                    return null;
                }

                @Override
                public ProviderPricingOptions getPricingOptions(ProviderContentItem item, @Nullable String session) {
                    // mocked using spy
                    return null;
                }

                @Override
                public AvailabilityInfo getAvailability(ProviderContentItem providerContentItem,
                                                        @Nullable String session, String userIp, String userAgent,
                                                        boolean withStream) {
                    return AvailabilityInfo.unavailable(true, List.of(createPricingOption(getProviderName(),
                                    new BigDecimal(100), PricingOptionType.BUY, providerContentItem)),
                            RejectionReason.PURCHASE_NOT_FOUND);
                }

                @Override
                public void processPurchase(ProviderContentItem purchasingItem, PricingOption selectedOption,
                                            String transactionId, String session) throws ProviderPurchaseException {

                }

                @Override
                public ProviderPromoCodeActivationResult activatePromoCode(String promoCode, String session)
                        throws PromoCodeActivationException {
                    throw new PromoCodeActivationException();
                }

                @Override
                public Map<ProviderContentItem, ProviderActiveSubscriptionInfo>
                getActiveSubscriptions(@Nonnull String session) {
                    return Collections.emptyMap();
                }

                @Override
                public Integer getPriority() {
                    return 1;
                }

                @Override
                public boolean showInYandexApp() {
                    return true;
                }
            };
        }

    }

}
