package ru.yandex.quasar.billing.dao;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Optional;
import java.util.UUID;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.jdbc.core.RowMapper;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.quasar.billing.util.HasCode;

@ReadOnlyTransactional
public interface ProductTokenDao extends Repository<ProductToken, Long> {
    @Query(
            value = "select\n" +
            "    pt.id as id,\n" +
            "    pt.provider as provider,\n" +
            "    pt.code as code,\n" +
            "    pt.reusable as reusable,\n" +
            "    pt.skill_uuid as skill_uuid,\n" +
            "    pt.product_uuid as product_uuid,\n" +
            "    sp.name as product_name,\n" +
            "    sp.type as product_type,\n" +
            "    sp.price as product_price,\n" +
            "    sp.deleted as product_deleted\n" +
            "from product_token pt\n" +
            "join skill_product sp\n" +
            "    on pt.product_uuid = sp.uuid\n" +
            "        and pt.skill_uuid = sp.skill_uuid\n" +
            "where pt.code = :code\n" +
            "    and pt.skill_uuid = :skill_uuid",
            rowMapperClass = ProductTokenRowMapper.class
    )
    Optional<ProductToken> getProductToken(@Param("code") String code, @Param("skill_uuid") String skillUuid);

    class ProductTokenRowMapper implements RowMapper<ProductToken> {
        @Override
        public ProductToken mapRow(ResultSet rs, int rowNum) throws SQLException {
            return new ProductToken(
                    rs.getLong("id"),
                    rs.getString("provider"),
                    rs.getString("code"),
                    rs.getBoolean("reusable"),
                    new SkillProduct(
                            UUID.fromString(rs.getString("product_uuid")),
                            rs.getString("skill_uuid"),
                            rs.getString("product_name"),
                            HasCode.getByCode(SkillProductType.class, rs.getString("product_type")),
                            rs.getBigDecimal("product_price"),
                            rs.getBoolean("product_deleted")
                    )
            );
        }
    }
}
