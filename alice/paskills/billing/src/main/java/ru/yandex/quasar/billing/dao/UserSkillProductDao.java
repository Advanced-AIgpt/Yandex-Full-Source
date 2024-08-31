package ru.yandex.quasar.billing.dao;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nullable;

import org.springframework.data.jdbc.repository.query.Modifying;
import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.jdbc.core.RowMapper;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.services.UserSkillProductActivationData;
import ru.yandex.quasar.billing.util.HasCode;

@ReadOnlyTransactional
public interface UserSkillProductDao extends Repository<UserSkillProduct, Long> {
    @Query(
            value = "select\n" +
                    "    usp.id as id,\n" +
                    "    usp.uid as uid,\n" +
                    "    usp.skill_name as skill_name,\n" +
                    "    usp.skill_image_url as skill_image_url,\n" +
                    "    sp.uuid as product_uuid,\n" +
                    "    sp.skill_uuid as skill_uuid,\n" +
                    "    sp.name as product_name,\n" +
                    "    sp.type as product_type,\n" +
                    "    sp.price as product_price,\n" +
                    "    sp.deleted as product_deleted,\n" +
                    "    pt.id as token_id,\n" +
                    "    pt.provider as token_provider,\n" +
                    "    pt.code as token_code,\n" +
                    "    pt.reusable as token_reusable\n" +
                    "from user_skill_product usp\n" +
                    "join skill_product sp\n" +
                    "    on usp.product_uuid = sp.uuid\n" +
                    "        and usp.skill_uuid = sp.skill_uuid\n" +
                    "left join product_token pt\n" +
                    "    on usp.token_id = pt.id\n" +
                    "where uid = :uid\n" +
                    "    and sp.skill_uuid = :skill_uuid",
            rowMapperClass = UserSkillProductRowMapper.class
    )
    List<UserSkillProduct> getUserSkillProducts(@Param("uid") String uid, @Param("skill_uuid") String skillUuid);

    @Query(
            value = "select\n" +
                    "    usp.id as id,\n" +
                    "    usp.uid as uid,\n" +
                    "    usp.skill_name as skill_name,\n" +
                    "    usp.skill_image_url as skill_image_url,\n" +
                    "    sp.uuid as product_uuid,\n" +
                    "    sp.skill_uuid as skill_uuid,\n" +
                    "    sp.name as product_name,\n" +
                    "    sp.type as product_type,\n" +
                    "    sp.price as product_price,\n" +
                    "    sp.deleted as product_deleted,\n" +
                    "    pt.id as token_id,\n" +
                    "    pt.provider as token_provider,\n" +
                    "    pt.code as token_code,\n" +
                    "    pt.reusable as token_reusable\n" +
                    "from user_skill_product usp\n" +
                    "join skill_product sp\n" +
                    "    on usp.product_uuid = sp.uuid\n" +
                    "        and usp.skill_uuid = sp.skill_uuid\n" +
                    "left join product_token pt\n" +
                    "    on usp.token_id = pt.id\n" +
                    "where uid = :uid\n" +
                    "    and sp.skill_uuid = :skill_uuid\n" +
                    "    and sp.uuid = :product_uuid",
            rowMapperClass = UserSkillProductRowMapper.class
    )
    Optional<UserSkillProduct> getUserSkillProduct(
            @Param("uid") String uid,
            @Param("skill_uuid") String skillUuid,
            @Param("product_uuid") UUID productUuid
    );

    @Query(
            value = "select\n" +
                    "    usp.id as id,\n" +
                    "    usp.uid as uid,\n" +
                    "    usp.skill_name as skill_name,\n" +
                    "    usp.skill_image_url as skill_image_url,\n" +
                    "    sp.uuid as product_uuid,\n" +
                    "    sp.skill_uuid as skill_uuid,\n" +
                    "    sp.name as product_name,\n" +
                    "    sp.type as product_type,\n" +
                    "    sp.price as product_price,\n" +
                    "    sp.deleted as product_deleted,\n" +
                    "    pt.id as token_id,\n" +
                    "    pt.provider as token_provider,\n" +
                    "    pt.code as token_code,\n" +
                    "    pt.reusable as token_reusable\n" +
                    "from user_skill_product usp\n" +
                    "join skill_product sp\n" +
                    "    on usp.product_uuid = sp.uuid\n" +
                    "        and usp.skill_uuid = sp.skill_uuid\n" +
                    "left join product_token pt\n" +
                    "    on usp.token_id = pt.id\n" +
                    "where usp.id = :id",
            rowMapperClass = UserSkillProductRowMapper.class
    )
    Optional<UserSkillProduct> getUserSkillProductById(@Param("id") Long id);

    @ReadWriteTransactional
    default void createUserSkillProduct(
            UserSkillProduct userSkillProduct,
            UserSkillProductActivationData activationData
    ) {
        ProductToken productToken = userSkillProduct.getProductToken();
        Long tokenId = productToken != null ? productToken.getId() : null;

        var skillProduct = userSkillProduct.getSkillProduct();
        String uid = userSkillProduct.getUid();
        String skillUuid = skillProduct.getSkillUuid();
        UUID productUuid = skillProduct.getUuid();

        createUserSkillProduct(uid, tokenId, skillUuid, productUuid, activationData.getSkillName(),
                activationData.getSkillImageUrl());
    }

    @Modifying
    @ReadWriteTransactional
    @Query(
            "insert into user_skill_product (\n" +
                    "    uid,\n" +
                    "    token_id,\n" +
                    "    product_uuid,\n" +
                    "    skill_uuid,\n" +
                    "    skill_name,\n" +
                    "    skill_image_url\n" +
                    ") values (\n" +
                    "    :uid,\n" +
                    "    :token_id,\n" +
                    "    :product_uuid,\n" +
                    "    :skill_uuid,\n" +
                    "    :skill_name,\n" +
                    "    :skill_image_url\n" +
                    ")"
    )
    void createUserSkillProduct(
            @Param("uid") String uid,
            @Nullable @Param("token_id") Long tokenId,
            @Param("skill_uuid") String skillUuid,
            @Param("product_uuid") UUID productUuid,
            @Param("skill_name") String skillName,
            @Param("skill_image_url") String skillImageUrl
    );

    @Modifying
    @ReadWriteTransactional
    @Query(
            "delete from user_skill_product\n" +
                    "where uid = :uid\n" +
                    "   and product_uuid = :product_uuid\n" +
                    "   and skill_uuid = :skill_uuid"
    )
    void deleteUserSkillProduct(
            @Param("uid") String uid,
            @Param("skill_uuid") String skillUuid,
            @Param("product_uuid") UUID productUuid
    );

    class UserSkillProductRowMapper implements RowMapper<UserSkillProduct> {
        @Override
        public UserSkillProduct mapRow(ResultSet rs, int rowNum) throws SQLException {
            SkillProduct skillProduct = new SkillProduct(
                    UUID.fromString(rs.getString("product_uuid")),
                    rs.getString("skill_uuid"),
                    rs.getString("product_name"),
                    HasCode.getByCode(SkillProductType.class, rs.getString("product_type")),
                    rs.getBigDecimal("product_price"),
                    rs.getBoolean("product_deleted")
            );
            return new UserSkillProduct(
                    rs.getLong("id"),
                    rs.getString("uid"),
                    createProductToken(rs, skillProduct),
                    skillProduct,
                    rs.getString("skill_name"),
                    rs.getString("skill_image_url")
            );
        }
    }

    @Nullable
    private static ProductToken createProductToken(ResultSet rs, SkillProduct skillProduct) throws SQLException {
        long tokenId = rs.getLong("token_id");
        if (rs.wasNull()) {
            return null;
        }
        return new ProductToken(
                tokenId,
                rs.getString("token_provider"),
                rs.getString("token_code"),
                rs.getBoolean("token_reusable"),
                skillProduct
        );
    }
}
