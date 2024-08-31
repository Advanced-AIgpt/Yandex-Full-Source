package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.junit.jupiter.SpringJUnitConfig;
import org.springframework.transaction.PlatformTransactionManager;

import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.beans.PricingOptionTestUtil;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.quasar.billing.services.processing.trust.PostClearingJob.DAYS_BEFORE_CLEARING_MUST_BE_FINISHED;

@SpringJUnitConfig
class PostClearingJobTest implements PricingOptionTestUtil {
    @MockBean
    private PlatformTransactionManager transactionManager;

    @MockBean
    private UserPurchasesDAO userPurchasesDAO;
    @MockBean
    private ProcessingService processingService;
    @MockBean
    private ProcessingServiceManager manager;
    private PostClearingJob job;

    @BeforeEach
    void setUp() {
        when(manager.get(PaymentProcessor.TRUST)).thenReturn(processingService);
        job = new PostClearingJob(manager, userPurchasesDAO, transactionManager);
        when(userPurchasesDAO.findAllWaitingForClearing(anyLong(), anyInt()))
                .thenReturn(List.of(PurchaseInfo.createSinglePayment(1L,
                        999L,
                        "token",
                        ProviderContentItem.createSeason("1"),
                        createPricingOption("test", BigDecimal.TEN, ProviderContentItem.createSeason("1")),
                        PurchaseInfo.Status.WAITING_FOR_CLEARING,
                        "",
                        1L,
                        null,
                        "test",
                        null,
                        PaymentProcessor.TRUST,
                        null,
                        null)))
                .thenReturn(List.of());
    }

    @Test
    void testCleared() {
        PaymentInfo paymentInfo = getPaymentInfo(PaymentInfo.Status.CLEARED, Instant.now(), Instant.now());
        when(processingService.getPaymentShortInfo(eq("token"), eq("999"), anyString()))
                .thenReturn(paymentInfo);
        // when
        job.postClearing();

        // then
        verify(userPurchasesDAO).updateClearedStatus(eq(1L), eq(paymentInfo.getClearDate()));
    }

    @Test
    void testNotYetCleared() {
        PaymentInfo paymentInfo = getPaymentInfo(PaymentInfo.Status.AUTHORIZED, Instant.now().minus(1L,
                ChronoUnit.DAYS), null);
        when(processingService.getPaymentShortInfo(eq("token"), eq("999"), anyString()))
                .thenReturn(paymentInfo);
        // when
        job.postClearing();

        // then
        verify(userPurchasesDAO, times(0)).updatePurchaseStatus(eq(1L), any());
    }

    @Test
    void testNotCleared() {
        PaymentInfo paymentInfo = getPaymentInfo(PaymentInfo.Status.AUTHORIZED,
                Instant.now().minus(DAYS_BEFORE_CLEARING_MUST_BE_FINISHED + 1L, ChronoUnit.DAYS), null);
        when(processingService.getPaymentShortInfo(eq("token"), eq("999"), anyString()))
                .thenReturn(paymentInfo);
        // when
        job.postClearing();

        // then
        verify(userPurchasesDAO).updatePurchaseStatus(eq(1L), eq(PurchaseInfo.Status.NOT_CLEARED));
    }

    @Nonnull
    private PaymentInfo getPaymentInfo(PaymentInfo.Status status, Instant paymentDate, @Nullable Instant clearDate) {
        return new PaymentInfo(
                "token",
                "card-xxx",
                "12123123123123",
                BigDecimal.TEN,
                "RUB",
                paymentDate,
                "MasterCard",
                status,
                clearDate
        );
    }
}
