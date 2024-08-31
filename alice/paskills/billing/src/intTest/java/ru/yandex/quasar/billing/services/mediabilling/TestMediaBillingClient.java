package ru.yandex.quasar.billing.services.mediabilling;

import java.math.BigDecimal;
import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneOffset;
import java.time.temporal.ChronoUnit;
import java.util.Currency;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicLong;

import javax.annotation.Nullable;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Primary;
import org.springframework.stereotype.Component;

import ru.yandex.quasar.billing.providers.IntegrationTestProvider;
import ru.yandex.quasar.billing.services.processing.trust.TemplateTag;
import ru.yandex.quasar.billing.services.promo.PromoInfo;

/**
 * Mocking MediaBilling client for integration tests
 */
@Component
@Primary
public class TestMediaBillingClient implements MediaBillingClient {

    public static final String YA_PREMIUM_SUB_ID = "ru.yandex.kinopoisk.amediateka.30min.autorenewable.native.web" +
            ".10min.trial.plus";

    private final Map<String, Map.Entry<OrderInfo, String>> orders = new HashMap<>();
    private final AtomicLong orderSequence = new AtomicLong();
    private final Map<String, String> activations = new HashMap<>();

    @Autowired
    @Qualifier("integrationTestProvider")
    private IntegrationTestProvider provider;

    @Override
    public PromoCodeActivationResult activatePromoCode(long uid, String code, String paymentMethodId,
                                                       String origin, @Nullable Integer region) {
        Objects.requireNonNull(code);
        Objects.requireNonNull(paymentMethodId);
        if (!activations.containsKey(String.valueOf(uid))) {
            activations.put(String.valueOf(uid), code);
            return new PromoCodeActivationResult(MusicPromoActivationResult.SUCCESS);
        } else {
            return new PromoCodeActivationResult(MusicPromoActivationResult.CODE_ALREADY_CONSUMED);
        }
    }

    @Override
    public PromoInfo prototypeFeatures(long uid, String prototype,
                                       @Nullable String platform,
                                       @Nullable Integer region) {
        return new PromoInfo.Subscription(YA_PREMIUM_SUB_ID, 5, BigDecimal.valueOf(699), Currency.getInstance("RUB"),
                LocalDate.of(2022, 6, 1).atStartOfDay().toInstant(ZoneOffset.UTC));
    }

    @Override
    public SubmitNativeOrderResult submitNativeOrder(String uid, String productId, String paymentCardId) {
        if (YA_PREMIUM_SUB_ID.equals(productId)) {
            OrderInfo orderInfo = new OrderInfo(orderSequence.incrementAndGet(),
                    BigDecimal.ZERO,
                    BigDecimal.ZERO,
                    "RUB",
                    Instant.now(),
                    "native-auto-subscription",
                    BigDecimal.ZERO,
                    BigDecimal.ONE,
                    true,
                    OrderInfo.OrderStatus.OK);
            Map.Entry<OrderInfo, String> entry = Map.entry(orderInfo, UUID.randomUUID().toString());
            orders.put(orderInfo.getOrderId().toString(), entry);

            provider.getPurchasedSubscriptions().put(IntegrationTestProvider.YA_SUBSCRIPTION_PREMIUM,
                    Instant.now().plus(5, ChronoUnit.DAYS));
            return SubmitNativeOrderResult.success(orderInfo.getOrderId(), entry.getValue());

        }
        return null;
    }

    @Override
    public String clonePrototype(String prototype) {
        return prototype + "_clone";
    }

    @Override
    public OrderInfo getOrderInfo(String uid, String orderId) {
        return orders.get(orderId).getKey();
    }

    @Override
    public BindCardResult bindCard(String uid, String userIp, String backUrl, TemplateTag template) {
        return null;
    }

    public Map<String, String> getActivations() {
        return activations;
    }

    public Map<String, Map.Entry<OrderInfo, String>> getOrders() {
        return orders;
    }

    public void clear() {
        activations.clear();
        orders.clear();
    }
}
