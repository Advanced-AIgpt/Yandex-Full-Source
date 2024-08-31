package ru.yandex.quasar.billing.services;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.stereotype.Service;

import ru.yandex.quasar.billing.dao.PurchaseInfo;
import ru.yandex.quasar.billing.dao.PurchaseOffer;
import ru.yandex.quasar.billing.dao.PurchaseOfferDao;
import ru.yandex.quasar.billing.dao.SubscriptionInfo;
import ru.yandex.quasar.billing.dao.UsedDevicePromo;
import ru.yandex.quasar.billing.dao.UsedDevicePromoDao;
import ru.yandex.quasar.billing.dao.UserPromoCode;
import ru.yandex.quasar.billing.dao.UserPromoCodeDao;
import ru.yandex.quasar.billing.dao.UserPurchasesDAO;
import ru.yandex.quasar.billing.dao.UserSubscriptionsDAO;
import ru.yandex.quasar.billing.util.ParallelHelper;

@Service
public class TakeoutService {

    static final String PURCHASE_OFFER = "purchase_offer";
    static final String USED_DEVICE_PROMO = "used_device_promo";
    static final String USER_PROMO_CODE = "user_promo_code";
    static final String USER_PURCHASES = "user_purchases";
    static final String USER_SUBSCRIPTIONS = "user_subscriptions";
    private final PurchaseOfferDao purchaseOfferDao;
    private final UsedDevicePromoDao usedDevicePromoDao;
    private final UserPromoCodeDao userPromoCodeDao;
    private final UserPurchasesDAO userPurchasesDAO;
    private final UserSubscriptionsDAO userSubscriptionsDAO;
    private final ParallelHelper helper;

    public TakeoutService(
            PurchaseOfferDao purchaseOfferDao,
            UsedDevicePromoDao usedDevicePromoDao,
            UserPromoCodeDao userPromoCodeDao,
            UserPurchasesDAO userPurchasesDAO,
            UserSubscriptionsDAO userSubscriptionsDAO,
            @Qualifier("takeoutServiceExecutor") ExecutorService executor,
            AuthorizationContext authorizationContext) {
        this.purchaseOfferDao = purchaseOfferDao;
        this.usedDevicePromoDao = usedDevicePromoDao;
        this.userPromoCodeDao = userPromoCodeDao;
        this.userPurchasesDAO = userPurchasesDAO;
        this.userSubscriptionsDAO = userSubscriptionsDAO;
        helper = new ParallelHelper(executor, authorizationContext);
    }

    public Map<String, List<?>> getUserData(String uid) {
        Map<String, List<?>> userData = new HashMap<>();

        CompletableFuture<List<PurchaseOffer>> purchaseOffers = helper.async(() -> purchaseOfferDao.findAllByUid(uid));
        CompletableFuture<List<UsedDevicePromo>> devicePromos =
                helper.async(() -> usedDevicePromoDao.findAllByUid(uid));
        CompletableFuture<List<UserPromoCode>> promoCodes =
                helper.async(() -> userPromoCodeDao.findAllByUid(Long.valueOf(uid)));
        CompletableFuture<List<PurchaseInfo>> purchases =
                helper.async(() -> userPurchasesDAO.findAllByUid(Long.valueOf(uid)));
        CompletableFuture<List<SubscriptionInfo>> subscriptions =
                helper.async(() -> userSubscriptionsDAO.findAllByUid(Long.valueOf(uid)));

        userData.put(PURCHASE_OFFER, purchaseOffers.join());
        userData.put(USED_DEVICE_PROMO, devicePromos.join());
        userData.put(USER_PROMO_CODE, promoCodes.join());
        userData.put(USER_PURCHASES, purchases.join());
        userData.put(USER_SUBSCRIPTIONS, subscriptions.join());

        return userData;
    }

    @Configuration
    static class ContentServiceExecutorConfig {
        @Bean(value = "takeoutServiceExecutor", destroyMethod = "shutdownNow")
        public ExecutorService contentExecutorService() {
            return Executors.newCachedThreadPool(
                    new ThreadFactoryBuilder()
                            .setNameFormat("takeout-%d")
                            .build()
            );
        }
    }
}
