package ru.yandex.quasar.billing.dao;

import java.util.List;
import java.util.Optional;

import org.springframework.data.jdbc.repository.query.Modifying;
import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.CrudRepository;
import org.springframework.data.repository.query.Param;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;


public interface PurchaseOfferDao extends CrudRepository<PurchaseOffer, Long> {

    @ReadOnlyTransactional
    @Override
    @Query("select * from purchase_offer where purchase_offer_id = :id")
    Optional<PurchaseOffer> findById(Long id);

    @ReadOnlyTransactional
    @Query("select * from purchase_offer where skill_info_id = :skill_info_id and purchase_request_id = " +
            ":purchase_request_id")
    Optional<PurchaseOffer> findBySkillIdAndPurchaseRequestId(@Param("skill_info_id") Long skillInfoId, @Param(
            "purchase_request_id") String purchaseRequestId);

    @ReadOnlyTransactional
    @Query("select\n" +
            "   * from purchase_offer\n" +
            "where skill_info_id = :skill_info_id\n" +
            "   and purchase_offer_id = :purchase_offer_id")
    Optional<PurchaseOffer> findBySkillIdAndPurchaseOfferId(
            @Param("skill_info_id") Long skillInfoId,
            @Param("purchase_offer_id") Long purchaseOfferId
    );

    @ReadOnlyTransactional
    @Query("select * from purchase_offer where uid = :uid and uuid = :uuid")
    Optional<PurchaseOffer> findByUidAndUuid(@Param("uid") String uid, @Param("uuid") String uuid);

    @ReadOnlyTransactional
    @Query("select * from purchase_offer where uid = :uid")
    List<PurchaseOffer> findAllByUid(@Param("uid") String uid);

    @Modifying
    @ReadWriteTransactional
    @Query("update purchase_offer set uid = :uid where uuid = :uuid and status = 'NEW' and (uid is null or uid = :uid)")
    boolean bindToUser(@Param("uuid") String uuid, @Param("uid") String uid);

}
