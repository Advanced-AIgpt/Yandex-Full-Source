package ru.yandex.quasar.billing.dao;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;

import javax.validation.constraints.NotEmpty;

import org.springframework.data.jdbc.repository.query.Query;
import org.springframework.data.repository.Repository;
import org.springframework.data.repository.query.Param;
import org.springframework.jdbc.core.RowMapper;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.quasar.billing.util.HasCode;

@ReadOnlyTransactional
public interface SkillProductDao extends Repository<SkillProduct, UUID> {
    @Query(
            value = "select \n" +
                    "    uuid,\n" +
                    "    skill_uuid,\n" +
                    "    name,\n" +
                    "    type,\n" +
                    "    price,\n" +
                    "    deleted\n" +
                    "from skill_product\n" +
                    "where uuid = :product_uuid\n" +
                    "    and skill_uuid = :skill_uuid",
            rowMapperClass = SkillProductRowMapper.class
    )
    Optional<SkillProduct> getSkillProduct(
            @Param("product_uuid") UUID productUuid,
            @Param("skill_uuid") String skillUuid
    );

    @Query(
            value = "select \n" +
                    "    uuid,\n" +
                    "    skill_uuid,\n" +
                    "    name,\n" +
                    "    type,\n" +
                    "    price,\n" +
                    "    deleted\n" +
                    "from skill_product\n" +
                    "where uuid in (:product_uuids)\n" +
                    "    and skill_uuid = :skill_uuid",
            rowMapperClass = SkillProductRowMapper.class
    )
    List<SkillProduct> getSkillProducts(
            @NotEmpty @Param("product_uuids") Set<UUID> productUuids,
            @Param("skill_uuid") String skillUuid
    );

    class SkillProductRowMapper implements RowMapper<SkillProduct> {
        @Override
        public SkillProduct mapRow(ResultSet rs, int rowNum) throws SQLException {
            return new SkillProduct(
                    UUID.fromString(rs.getString("uuid")),
                    rs.getString("skill_uuid"),
                    rs.getString("name"),
                    HasCode.getByCode(SkillProductType.class, rs.getString("type")),
                    rs.getBigDecimal("price"),
                    rs.getBoolean("deleted")
            );
        }
    }
}
