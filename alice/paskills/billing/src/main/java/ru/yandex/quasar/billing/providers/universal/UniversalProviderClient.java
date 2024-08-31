package ru.yandex.quasar.billing.providers.universal;

import javax.annotation.Nullable;

import ru.yandex.quasar.billing.providers.StreamData;

public interface UniversalProviderClient {

    AllContentItems allContent();

    ContentItemInfo contentItemInfo(String contentItemId, @Nullable String session);

    ContentAvailable contentAvailable(String contentItemId, boolean requestStream, @Nullable String session,
                                      String userIp, String userAgent);


    ProductItems contentPurchaseOptions(String contentItemId, @Nullable String session);


    StreamData contentStream(String contentItemId, @Nullable String session, String userIp, String userAgent);


    ProductItem productInfo(String productId, @Nullable String session);


    ProductPrice productPriceInfo(String productId, String priceId, @Nullable String session);


    PurchaseProcessResult purchaseProduct(String productId, String priceId, PurchaseRequest purchaseRequest,
                                          String session);



    PurchasedItems listPurchasedItems(@Nullable ProductType type, String session);


    PromoCodeResult activatePromoCode(String promocode, String session);
}
