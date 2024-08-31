package ru.yandex.quasar.billing.services.processing.trust;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.springframework.transaction.PlatformTransactionManager;
import org.springframework.transaction.support.TransactionTemplate;

import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.processing.ProcessingService;
import ru.yandex.quasar.billing.services.processing.ProcessingServiceManager;

@Component
class PostClearingJob {

    static final long DAYS_BEFORE_CLEARING_MUST_BE_FINISHED = 3L;
    private static final Logger log = LogManager.getLogger();
    private final ProcessingServiceManager serviceManager;
    private final UserPurchasesDAO userPurchasesDAO;
    private final TransactionTemplate transactionTemplate;

    PostClearingJob(
            ProcessingServiceManager serviceManager,
            UserPurchasesDAO userPurchasesDAO,
            PlatformTransactionManager transactionManager
    ) {
        this.serviceManager = serviceManager;
        this.userPurchasesDAO = userPurchasesDAO;
        this.transactionTemplate = new TransactionTemplate(transactionManager);
    }

    @Scheduled(initialDelay = 60_000L, fixedDelay = 600_000L)
    void postClearing() {
        transactionTemplate.execute(status -> {
                    List<PurchaseInfo> purchases;
                    long maxPurchaseId = Long.MIN_VALUE;
                    do {
                        purchases = userPurchasesDAO.findAllWaitingForClearing(maxPurchaseId, 20);
                        purchases.forEach(this::process);
                        maxPurchaseId = purchases.stream()
                                .mapToLong(PurchaseInfo::getPurchaseId)
                                .max()
                                .orElse(Long.MIN_VALUE);
                    } while (!purchases.isEmpty());
                    return null;
                }
        );

    }

    private void process(PurchaseInfo purchase) {
        ProcessingService processingService = serviceManager.get(purchase.getPaymentProcessor());
        PaymentInfo paymentShortInfo = processingService.getPaymentShortInfo(purchase.getPurchaseToken(),
                purchase.getUid().toString(), "127.0.0.1");

        if (paymentShortInfo.getClearDate() != null) {
            userPurchasesDAO.updateClearedStatus(purchase.getPurchaseId(), paymentShortInfo.getClearDate());
            log.info("Purchase {} cleared", purchase.getPurchaseId());
        } else {
            boolean timedOut = Instant.now()
                    .isAfter(paymentShortInfo.getPaymentDate().plus(DAYS_BEFORE_CLEARING_MUST_BE_FINISHED,
                            ChronoUnit.DAYS));
            if (timedOut) {
                log.info("Purchase {} not cleared, timed out", purchase.getPurchaseId());
                userPurchasesDAO.updatePurchaseStatus(purchase.getPurchaseId(), PurchaseInfo.Status.NOT_CLEARED);
            }
        }
    }
}
