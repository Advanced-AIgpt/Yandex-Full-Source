package ru.yandex.quasar.billing.services.promo;

import java.util.Collection;

/**
 * Service for promo activities for new Yandex devices.
 * Service is responsible for granting different promo subscriptions odder with device purchase
 */
public interface QuasarPromoService {

    /**
     * Get information about all user devices with their promo availability
     *
     * @param uid user identifier
     * @return List of objects with device information
     * @throws QuasarBackendException if something happened when accessing device list
     */
    Collection<DeviceInfo> getUserDevices(String uid);

    /**
     * Get information of the user device with its promo availability
     *
     * @param uid      user identifier
     * @param deviceId composite device identifier
     * @return List of objects with device information
     * @throws DeviceNotOwnedByUserException if device is not owned by the user
     */
    DeviceInfo getUserDevice(String uid, DeviceId deviceId) throws DeviceNotOwnedByUserException;

    DeviceInfo getUserDevice(String uid, BackendDeviceInfo backendDevice)
            throws DeviceNotOwnedByUserException;

    /**
     * Activate promo period of given device to the given user
     *
     * @param promoProvider
     * @param uid           user identifier
     * @param userIp        user ip for trust request
     * @param deviceId      device identifier
     * @throws DeviceNotOwnedByUserException   if the device is not owned by the user
     * @throws DevicePromoAlreadyUsedException if promo is not available for the device
     */
    @SuppressWarnings("ParameterNumber")
    DevicePromoActivationResult activatePromoPeriodFromPP(
            PromoProvider promoProvider,
            String uid,
            String userIp,
            DeviceId deviceId,
            String paymentCardId);

    @SuppressWarnings("ParameterNumber")
    DevicePromoActivationResult activatePromoPeriodFromTv(
            PromoProvider provider,
            String uid,
            String userIp,
            BackendDeviceInfo backendDevice,
            String paymentCardId
    );

    /**
     * Request user to purchase Plus subscription or activate promo period
     *
     * @param uid user identifier
     * @return promo state, describing how the user may obtain subscription either by purchasing or activating promo
     * period from device
     */
    RequestPlusResult requestPlus(String uid);

    RequestPlusResult requestPlus(String uid, boolean sendPersonalCards);

}
