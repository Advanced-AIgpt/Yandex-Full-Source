package ru.yandex.quasar.billing.controller;

import java.util.function.Function;

import javax.annotation.Nullable;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.passport.tvmauth.CheckedServiceTicket;
import ru.yandex.passport.tvmauth.TicketStatus;
import ru.yandex.passport.tvmauth.TvmClient;
import ru.yandex.quasar.billing.beans.PaymentProcessor;
import ru.yandex.quasar.billing.config.BillingConfig;
import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.exception.BadRequestException;
import ru.yandex.quasar.billing.exception.NotFoundException;
import ru.yandex.quasar.billing.exception.UnauthorizedException;
import ru.yandex.quasar.billing.filter.TvmRequired;
import ru.yandex.quasar.billing.services.AuthorizationContext;
import ru.yandex.quasar.billing.services.PaymentInfo;
import ru.yandex.quasar.billing.services.RefundInfo;
import ru.yandex.quasar.billing.services.processing.trust.RefundException;
import ru.yandex.quasar.billing.services.processing.trust.TrustBillingService;
import ru.yandex.quasar.billing.services.processing.trust.TrustRefundInfo;
import ru.yandex.quasar.billing.services.tvm.TvmClientName;
import ru.yandex.quasar.billing.services.tvm.TvmHeaders;

import static ru.yandex.quasar.billing.services.tvm.TvmClientName.ott_backoffice;

@RestController
@RequestMapping(path = "/billing/internal")
class InternalMethodsController {

    private final UserPurchasesDAO userPurchasesDAO;
    private final AuthorizationContext authorizationContext;
    private final TrustBillingService trustBillingService;
    private final TvmClient tvmClient;
    private final int selfTvmAlias;


    InternalMethodsController(UserPurchasesDAO userPurchasesDAO,
                              AuthorizationContext authorizationContext,
                              TrustBillingService trustBillingService,
                              TvmClient tvmClient,
                              BillingConfig config
    ) {
        this.userPurchasesDAO = userPurchasesDAO;
        this.authorizationContext = authorizationContext;
        this.trustBillingService = trustBillingService;
        this.tvmClient = tvmClient;
        this.selfTvmAlias = config.getTvmConfig().getClientId();
    }

    // handler can be called only from local host or by direct call by DNS name
    @PostMapping("refundPayment")
    @TvmRequired(TvmClientName.self)
    public ResponseEntity<RefundInfo> refundPayment(
            @RequestParam("purchaseToken") String purchaseToken,
            @RequestParam(value = "description", required = false) @Nullable String description) {

        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseToken)
                .orElseThrow(() -> new NotFoundException("Purchase not found for purchaseToken " + purchaseToken));
        authorizationContext.setCurrentUid(purchaseInfo.getUid().toString());

        try {
            RefundInfo refundInfo = trustBillingService.refundPayment(purchaseInfo.getUid().toString(),
                    purchaseInfo.getPurchaseToken(),
                    purchaseInfo.getSubscriptionId() != null ? purchaseInfo.getSubscriptionId().toString() : null,
                    description);
            return ResponseEntity.ok(refundInfo);
        } catch (RefundException e) {
            throw new BadRequestException(e.getMessage(), e);
        }

    }

    // not used any more
    @Deprecated(forRemoval = true)
    @GetMapping("payment/{purchaseToken}")
    @TvmRequired({TvmClientName.self, ott_backoffice})
    public PaymentInfo paymentInfo(
            @PathVariable("purchaseToken") String purchaseToken,
            @RequestHeader(name = TvmHeaders.SERVICE_TICKET_HEADER) String tvmServiceTicket
    ) {
        return checkAccess(purchaseToken, tvmServiceTicket, purchaseInfo ->
                trustBillingService.getPaymentShortInfo(purchaseToken, purchaseInfo.getUid().toString(),
                        "127.0.0.1")
        );
    }

    // not used any more
    @Deprecated(forRemoval = true)
    @PostMapping("payment/{purchaseToken}/refund")
    @TvmRequired({TvmClientName.self, ott_backoffice})
    public StartRefundResponse startRefund(
            @PathVariable("purchaseToken") String purchaseToken,
            @RequestHeader(name = TvmHeaders.SERVICE_TICKET_HEADER) String tvmServiceTicket
    ) {
        return checkAccess(purchaseToken, tvmServiceTicket, purchaseInfo -> {
            String refundId = trustBillingService.startRefund(
                    purchaseInfo.getUid().toString(),
                    purchaseToken,
                    purchaseInfo.getSubscriptionId() != null ? purchaseInfo.getSubscriptionId().toString() : null,
                    "ott backoffice request"
            );
            return new StartRefundResponse(refundId);
        });
    }

    private <T> T checkAccess(String purchaseToken, String tvmServiceTicket, Function<PurchaseInfo, T> func) {
        PurchaseInfo purchaseInfo = userPurchasesDAO.getPurchaseInfo(purchaseToken)
                .orElseThrow(() -> new NotFoundException("Purchase not found for purchaseToken " + purchaseToken));

        CheckedServiceTicket ticket = tvmClient.checkServiceTicket(tvmServiceTicket);
        if (ticket.getStatus() != TicketStatus.OK) {
            throw new UnauthorizedException("Wrong service ticket: " + ticket.getStatus());
        }

        if (purchaseInfo.getPaymentProcessor() == PaymentProcessor.TRUST) {
            return func.apply(purchaseInfo);
        } else {
            throw new NotFoundException("TRUST Purchase not found");
        }
    }

    // not used any more
    @Deprecated(forRemoval = true)
    @GetMapping("refund/{refundId}")
    @TvmRequired({TvmClientName.self, ott_backoffice})
    public RefundInfoResponse refundInfo(@PathVariable("refundId") String refundId) {
        TrustRefundInfo refundStatus = trustBillingService.getRefundStatus(refundId);
        return new RefundInfoResponse(
                refundStatus.getStatus(),
                refundStatus.getStatusDescription(),
                refundStatus.getFiscalReceiptUrl()
        );
    }

}
