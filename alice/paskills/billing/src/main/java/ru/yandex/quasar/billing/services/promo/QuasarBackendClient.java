package ru.yandex.quasar.billing.services.promo;

import java.util.Collection;
import java.util.Map;
import java.util.Set;

public interface QuasarBackendClient {
    /**
     * get user's device list from backend
     *
     * @param uid user identifier
     * @return collection of device information
     * @throws QuasarBackendException if something happened when accessing the backend
     */
    Collection<BackendDeviceInfo> getUserDeviceList(String uid);

    Map<String, Set<String>> getUserDeviceTags(String uid);
}
