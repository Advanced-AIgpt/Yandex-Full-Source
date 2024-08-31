package ru.yandex.quasar.billing.services.processing.trust;

import java.math.BigDecimal;
import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonInclude.Include;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import lombok.Getter;

import ru.yandex.quasar.billing.services.processing.NdsType;
import ru.yandex.quasar.billing.services.processing.TrustCurrency;


@JsonInclude(Include.NON_NULL)
@Getter
class CreateBasketRequest {
    private final BigDecimal amount;
    private final TrustCurrency currency;
    @JsonProperty("product_id")
    private final String productId;
    @JsonProperty("paymethod_id")
    private final String paymethodId;
    @JsonProperty("user_email")
    private final String userEmail;
    @JsonProperty("back_url")
    private final String callBackUrl;
    @JsonProperty("fiscal_nds")
    private final NdsType fiscalNds;
    @JsonProperty("fiscal_title")
    private final String fiscalTitle;
    @JsonProperty("fiscal_inn")
    private final String fiscalInn;
    @JsonProperty("commission_category")
    private final String commissionCategory;
    @JsonProperty("orders")
    private final List<Order> orders;

    @SuppressWarnings("ParameterNumber")
    private CreateBasketRequest(BigDecimal amount, TrustCurrency currency, String productId, String paymethodId,
                                String userEmail, String callBackUrl, NdsType fiscalNds, String fiscalTitle,
                                String commissionCategory, String fiscalInn) {
        this.amount = amount;
        this.currency = currency;
        this.productId = productId;
        this.paymethodId = paymethodId;
        this.userEmail = userEmail;
        this.callBackUrl = callBackUrl;
        this.fiscalNds = fiscalNds;
        this.fiscalTitle = fiscalTitle;
        this.commissionCategory = commissionCategory;
        this.fiscalInn = fiscalInn;
        this.orders = null;
    }

    private CreateBasketRequest(String paymethodId, String userEmail, String callBackUrl, List<Order> orders) {
        this.paymethodId = paymethodId;
        this.userEmail = userEmail;
        this.callBackUrl = callBackUrl;
        this.orders = orders;

        this.amount = null;
        this.currency = null;
        this.productId = null;
        this.fiscalNds = null;
        this.fiscalTitle = null;
        this.fiscalInn = null;
        this.commissionCategory = null;

    }

    private CreateBasketRequest(TrustCurrency currency, String paymethodId, String userEmail, String callBackUrl,
                                String commissionCategory, List<Order> orders) {
        this.currency = currency;
        this.paymethodId = paymethodId;
        this.userEmail = userEmail;
        this.callBackUrl = callBackUrl;
        this.commissionCategory = commissionCategory;
        this.orders = orders;

        this.amount = null;
        this.productId = null;
        this.fiscalNds = null;
        this.fiscalTitle = null;
        this.fiscalInn = null;
    }

    @SuppressWarnings("ParameterNumber")
    static CreateBasketRequest createSimpleBasket(BigDecimal amount, TrustCurrency currency, String productId,
                                                  String paymethodId, String userEmail, String callBackUrl,
                                                  NdsType fiscalNds, String fiscalTitle, String commissionCategory,
                                                  String fiscalInn) {
        return new CreateBasketRequest(amount, currency, productId, paymethodId, userEmail, callBackUrl, fiscalNds,
                fiscalTitle, commissionCategory, fiscalInn);
    }

    static CreateBasketRequest createMultiItemBasket(TrustCurrency currency, String paymethodId, List<Order> orders,
                                                     String userEmail, String callBackUrl, String commissionCategory) {
        return new CreateBasketRequest(currency, paymethodId, userEmail, callBackUrl, commissionCategory, orders);
    }

    static CreateBasketRequest createSubscriptionBasket(String paymethodId, String userEmail, String callBackUrl,
                                                        int qty, String orderId) {
        return new CreateBasketRequest(paymethodId, userEmail, callBackUrl,
                Collections.singletonList(new Order(orderId, BigDecimal.valueOf(qty),
                        null, null, null, null)));
    }

    @Data
    @JsonInclude(Include.NON_NULL)
    static class Order {
        @JsonProperty("order_id")
        private final String orderId;
        private final BigDecimal qty;
        @Nullable
        private final BigDecimal price;
        @Nullable
        @JsonProperty("fiscal_nds")
        private final NdsType fiscalNds;
        @Nullable
        @JsonProperty("fiscal_title")
        private final String fiscalTitle;
        @Nullable
        @JsonProperty("fiscal_inn")
        private final String fiscalInn;
    }
}
