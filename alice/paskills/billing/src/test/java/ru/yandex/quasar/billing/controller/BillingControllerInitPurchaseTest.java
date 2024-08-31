package ru.yandex.quasar.billing.controller;

import java.io.IOException;
import java.math.BigDecimal;
import java.util.Collections;
import java.util.Optional;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.hamcrest.CoreMatchers;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.http.HttpStatus;
import org.springframework.mock.web.MockHttpServletRequest;
import org.springframework.web.server.ResponseStatusException;

import ru.yandex.quasar.billing.EmbeddedPostgresExtension;
import ru.yandex.quasar.billing.beans.ContentItem;
import ru.yandex.quasar.billing.beans.ContentType;
import ru.yandex.quasar.billing.beans.LogicalPeriod;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.config.TestConfigProvider;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.providers.ProviderPricingOptions;
import ru.yandex.quasar.billing.providers.TestContentProvider;
import ru.yandex.quasar.billing.services.AuthorizationService;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(EmbeddedPostgresExtension.class)
@SpringBootTest(classes = {TestConfigProvider.class})
class BillingControllerInitPurchaseTest implements PricingOptionTestUtil {

    private static final String PURCHASE_TOKEN = "qwertyuiop";
    private static final String UID = "123";
    @Autowired
    private BillingController controller;
    @MockBean
    private AuthorizationService authorizationService;
    @SpyBean
    private UserPurchasesDAO userPurchasesDAO;
    @MockBean
    private ProcessingServiceManager processingServiceManager;
    @MockBean
    private TrustBillingService trustService;
    @SpyBean
    private TestContentProvider testContentProvider;
    private String providerName;
    private String socialName;

    private ProviderContentItem filmItem = ProviderContentItem.create(ContentType.MOVIE, "film_d");
    private ProviderContentItem subscriptionItem = ProviderContentItem.create(ContentType.SUBSCRIPTION,
            "subscription_ID");

    private ObjectMapper objectMapper = new ObjectMapper();

    @BeforeEach
    void setUp() {
        when(processingServiceManager.get(any())).thenReturn(trustService);
        when(trustService.getPaymentProcessor()).thenReturn(PaymentProcessor.TRUST);
        providerName = testContentProvider.getProviderName();
        socialName = testContentProvider.getSocialAPIServiceName();
        when(authorizationService.getSecretUid(any())).thenReturn(UID);
        when(authorizationService.getProviderTokenByUid(any(), any())).thenReturn(Optional.empty());
        when(authorizationService.getProviderTokenByUid(eq(UID), eq(socialName))).thenReturn(Optional.of(
                "providerToken"));
    }

    @Test
    void initPurchaseProcessOkFilm() throws IOException {
        ProviderContentItem providerContentItem = this.filmItem;

        ContentItem contentItem = new ContentItem(providerName, providerContentItem);
        PricingOption pricingOption = createPricingOption(providerContentItem, 99L);

        when(trustService.createPayment(eq(UID), any(Long.class), anyString(), anyString(), eq(pricingOption),
                anyString(), any(), anyString(), isNull()))
                .thenReturn(PURCHASE_TOKEN);

        doAnswer(invocation -> ProviderPricingOptions.create(false, Collections.singletonList(pricingOption), null))
                .when(testContentProvider).getPricingOptions(any(), any());

        InitPurchaseProcessResponse initPurchaseProcessResponse = controller.initPurchaseProcess(contentItem,
                objectMapper.writeValueAsString(pricingOption), "card", "email", new MockHttpServletRequest());

        assertEquals(PURCHASE_TOKEN, initPurchaseProcessResponse.getPurchaseToken());
    }

    @Nonnull
    private PricingOption createPricingOption(ProviderContentItem providerContentItem, long price) {
        return createPricingOption(providerName, BigDecimal.valueOf(price), providerContentItem);
    }

    @Nonnull
    private PricingOption createSubscriptionPricingOption(ProviderContentItem purchasingItem) {
        return createSubscriptionPricingOption(providerName, BigDecimal.valueOf(199L), purchasingItem,
                LogicalPeriod.ofDays(30));
    }

    @Test
    void initPurchaseProcessOkSubscription() throws IOException {
        ProviderContentItem providerContentItem = this.subscriptionItem;
        ProviderContentItem purchasingItem = this.subscriptionItem;

        ContentItem contentItem = new ContentItem(providerName, providerContentItem);
        PricingOption pricingOption = createSubscriptionPricingOption(purchasingItem);

        when(trustService.createSubscription(eq(UID), any(Long.class), anyString(), anyString(), eq(pricingOption),
                anyInt(), anyString(), any(), anyString(), eq(pricingOption.getProvider())))
                .thenReturn(PURCHASE_TOKEN);

        doAnswer(invocation -> ProviderPricingOptions.create(false, Collections.singletonList(pricingOption), null))
                .when(testContentProvider).getPricingOptions(any(), any());
        doReturn(Collections.emptyMap())
                .when(testContentProvider).getActiveSubscriptions("providerToken");

        InitPurchaseProcessResponse initPurchaseProcessResponse = controller.initPurchaseProcess(contentItem,
                objectMapper.writeValueAsString(pricingOption), "card", "email", new MockHttpServletRequest());

        assertEquals(PURCHASE_TOKEN, initPurchaseProcessResponse.getPurchaseToken());
    }

    /**
     * test case when second request is send while the first request is still processing purchase
     * to do so we make second request from inside the {@link org.mockito.Answers} of trust service processing
     */
    @Test
    void initPurchaseProcessOkConcurrencyProtection() throws IOException {
        ProviderContentItem providerContentItem = this.subscriptionItem;
        ProviderContentItem purchasingItem = this.subscriptionItem;

        ContentItem contentItem = new ContentItem(providerName, providerContentItem);
        PricingOption pricingOption = createSubscriptionPricingOption(purchasingItem);

        String selectedOptionStr = objectMapper.writeValueAsString(pricingOption);

        doAnswer(invocation -> ProviderPricingOptions.create(false, Collections.singletonList(pricingOption), null))
                .when(testContentProvider).getPricingOptions(any(), any());
        doReturn(Collections.emptyMap())
                .when(testContentProvider).getActiveSubscriptions("providerToken");


        // we start another request when the first one is still processing
        when(trustService.createSubscription(eq(UID), any(Long.class), anyString(), anyString(), eq(pricingOption),
                anyInt(), anyString(), any(), anyString(), eq(providerName)))
                .thenAnswer(invocation -> {
                    // concurrent same request
                    try {
                        controller.initPurchaseProcess(contentItem, selectedOptionStr, "card", "email",
                                new MockHttpServletRequest());
                        fail();
                    } catch (ResponseStatusException e) {
                        assertEquals(HttpStatus.CONFLICT, e.getStatus());
                    }
                    return PURCHASE_TOKEN;
                });


        InitPurchaseProcessResponse initPurchaseProcessResponse = controller.initPurchaseProcess(contentItem,
                selectedOptionStr, "card", "email", new MockHttpServletRequest());
        assertEquals(PURCHASE_TOKEN, initPurchaseProcessResponse.getPurchaseToken());
    }

    @Test
    void initPurchaseProcessWithoutAccount() throws IOException {
        ProviderContentItem providerContentItem = this.filmItem;
        ProviderContentItem purchasingItem = this.filmItem;

        ContentItem contentItem = new ContentItem(providerName, providerContentItem);
        PricingOption pricingOption = createPricingOption(purchasingItem, 99L);

        when(authorizationService.getProviderTokenByUid(eq(UID), eq(socialName))).thenReturn(Optional.empty());


        InitPurchaseProcessResponse initPurchaseProcessResponse = controller.initPurchaseProcess(contentItem,
                objectMapper.writeValueAsString(pricingOption), "card", "email", new MockHttpServletRequest());

        assertThat(initPurchaseProcessResponse.getPurchaseToken(), CoreMatchers.startsWith("ERR"));

        ArgumentCaptor<PurchaseInfo> captor = ArgumentCaptor.forClass(PurchaseInfo.class);
        verify(userPurchasesDAO).savePurchaseInfo(captor.capture());

        assertAll(
                () -> assertEquals(Long.valueOf(UID), captor.getValue().getUid()),
                () -> assertThat(captor.getValue().getPurchaseToken(), CoreMatchers.startsWith("ERR")),
                () -> assertEquals(providerContentItem, captor.getValue().getContentItem()),
                () -> assertEquals(pricingOption, captor.getValue().getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_NO_PROVIDER_ACC, captor.getValue().getStatus()),
                () -> assertNull(captor.getValue().getSubscriptionId()),
                () -> assertNull(captor.getValue().getSecurityToken())
        );
    }

    @Test
    void initPurchaseProcessObsoletePricingOption() throws IOException {
        ProviderContentItem providerContentItem = this.filmItem;
        ProviderContentItem purchasingItem = this.filmItem;

        ContentItem contentItem = new ContentItem(providerName, providerContentItem);
        PricingOption pricingOption = createPricingOption(purchasingItem, 99L);

        PricingOption anotherPricingOption = createPricingOption(purchasingItem, 199L);

        doReturn(ProviderPricingOptions.create(false, Collections.singletonList(anotherPricingOption), null))
                .when(testContentProvider).getPricingOptions(providerContentItem, "providerToken");

        InitPurchaseProcessResponse initPurchaseProcessResponse = controller.initPurchaseProcess(contentItem,
                objectMapper.writeValueAsString(pricingOption), "card", "email", new MockHttpServletRequest());

        assertThat(initPurchaseProcessResponse.getPurchaseToken(), CoreMatchers.startsWith("ERR"));

        ArgumentCaptor<PurchaseInfo> captor = ArgumentCaptor.forClass(PurchaseInfo.class);
        verify(userPurchasesDAO).savePurchaseInfo(captor.capture());

        assertAll(
                () -> assertEquals(Long.valueOf(UID), captor.getValue().getUid()),
                () -> assertThat(captor.getValue().getPurchaseToken(), CoreMatchers.startsWith("ERR")),
                () -> assertEquals(providerContentItem, captor.getValue().getContentItem()),
                () -> assertEquals(pricingOption, captor.getValue().getSelectedOption()),
                () -> assertEquals(PurchaseInfo.Status.ERROR_PAYMENT_OPTION_OBSOLETE, captor.getValue().getStatus()),
                () -> assertNull(captor.getValue().getSubscriptionId()),
                () -> assertNull(captor.getValue().getSecurityToken())
        );
    }

}
