package ru.yandex.quasar.billing.dao;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.dao.EmptyResultDataAccessException;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.core.ColumnMapRowMapper;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.core.RowMapper;
import org.springframework.jdbc.core.namedparam.NamedParameterJdbcTemplate;
import org.springframework.stereotype.Repository;

import ru.yandex.alice.library.routingdatasource.ReadOnlyTransactional;
import ru.yandex.alice.library.routingdatasource.ReadWriteTransactional;
import ru.yandex.quasar.billing.services.promo.DeviceId;
import ru.yandex.quasar.billing.services.promo.Platform;
import ru.yandex.quasar.billing.services.promo.PromoProvider;

import static java.util.Collections.emptyList;

@Repository
@ReadOnlyTransactional
class UsedDevicePromoDaoImpl implements UsedDevicePromoDao {

    private final JdbcTemplate jdbcTemplate;
    private final NamedParameterJdbcTemplate namedParameterJdbcTemplate;
    private final BeanPropertyRowMapper<UsedDevicePromo> rowMapper;
    private final PromoStatisticsRowMapper promoStatisticsRowMapper;

    UsedDevicePromoDaoImpl(JdbcTemplate jdbcTemplate, NamedParameterJdbcTemplate namedParameterJdbcTemplate,
                           ObjectMapper objectMapper) {
        this.jdbcTemplate = jdbcTemplate;
        this.namedParameterJdbcTemplate = namedParameterJdbcTemplate;
        this.rowMapper = new BillingBeanPropertyRowMapper<>(UsedDevicePromo.class, objectMapper);
        this.promoStatisticsRowMapper = new PromoStatisticsRowMapper();
    }

    @ReadWriteTransactional
    Long getNextId() {
        List<Map<String, Object>> purchaseIdRes = jdbcTemplate.query(
                "SELECT nextval('UsedDevicePromoSeq')",
                new ColumnMapRowMapper()
        );

        return (Long) purchaseIdRes.get(0).get("nextval");
    }

    @Override
    @ReadWriteTransactional
    public UsedDevicePromo save(UsedDevicePromo usedDevicePromo) {
        if (usedDevicePromo.getId() == null) {
            usedDevicePromo.setId(getNextId());
            jdbcTemplate.update("insert into UsedDevicePromo (id, uid, deviceId, platform, provider, codeid," +
                            "promoActivationTime) values (?, ?, ?, ?, ?, ?, ?)",
                    usedDevicePromo.getId(), usedDevicePromo.getUid(), usedDevicePromo.getDeviceId(),
                    usedDevicePromo.getPlatform().toString(), usedDevicePromo.getProvider().toString(),
                    usedDevicePromo.getCodeId(), usedDevicePromo.getPromoActivationTime() != null ?
                            Timestamp.from(usedDevicePromo.getPromoActivationTime()) : null);
        } else {
            jdbcTemplate.update("update UsedDevicePromo set uid = ?, deviceId = ?, platform = ?, provider = ?, codeid" +
                            " = ?,promoActivationTime = ? where id = ?",
                    usedDevicePromo.getUid(), usedDevicePromo.getDeviceId(), usedDevicePromo.getPlatform().toString(),
                    usedDevicePromo.getProvider().name(), usedDevicePromo.getCodeId(),
                    usedDevicePromo.getPromoActivationTime() != null ?
                            Timestamp.from(usedDevicePromo.getPromoActivationTime()) : null, usedDevicePromo.getId());
        }

        return usedDevicePromo;
    }

    @Override
    public List<UsedDevicePromo> findByDeviceIdAndPlatform(String deviceId, Platform platform) {
        return jdbcTemplate.query("select * from UsedDevicePromo where deviceId = ? and platform = ?",
                rowMapper,
                deviceId, platform.toString());
    }

    @Override
    public Optional<UsedDevicePromo> findByUidAndDeviceIdAndPlatformAndProvider(String deviceId, Platform platform,
                                                                                String provider) {
        try {
            return Optional.ofNullable(jdbcTemplate.queryForObject("select * from UsedDevicePromo where deviceId = ? " +
                            "and platform = ? and provider = ?",
                    rowMapper,
                    deviceId, platform.toString(), provider));
        } catch (EmptyResultDataAccessException e) {
            return Optional.empty();
        }
    }

    @Override
    public List<UsedDevicePromo> findByDevices(Set<DeviceId> deviceIds) {
        if (deviceIds.isEmpty()) {
            return emptyList();
        }

        List<Object[]> inParam = deviceIds.stream()
                .map(deviceId -> new Object[]{deviceId.getId(), deviceId.getPlatform().toString()})
                .collect(Collectors.toList());

        return namedParameterJdbcTemplate.query(
                "select * from UsedDevicePromo where (deviceid, platform) in (:ids) and provider in (:providers)",
                Map.of("ids", inParam,
                        // filter by correct providers otherwise rowMapper may fail deserializing incorrect string
                        // to enum
                        "providers",
                        Stream.of(PromoProvider.values()).map(PromoProvider::name).collect(Collectors.toList())
                ),
                rowMapper);
    }

    @Override
    public List<PromoStatistics> getPromoStatistics() {
        return namedParameterJdbcTemplate.query(
                "select\n" +
                        "    p.promotype as promotype,\n" +
                        "    p.provider as provider,\n" +
                        "    p.platform as platform,\n" +
                        "    pr.promocode_prototype_id as prototype_id,\n" +
                        "    count(p.id) as total_count,\n" +
                        "    count(p.id) - count(d.codeid) as left_count\n" +
                        "from promocodebase p\n" +
                        "left join useddevicepromo d\n" +
                        "    on p.id = d.codeid\n" +
                        "left join promocode_prototype pr\n" +
                        "    on pr.platform = p.platform and pr.promo_type = p.promotype\n" +
                        // deprecated promos contains '_', will be changed in PASKILLS-5231
                        "where p.provider not like '%\\_%'\n" +
                        "group by p.promotype, p.platform, p.provider, pr.promocode_prototype_id",
                Collections.emptyMap(),
                promoStatisticsRowMapper
        );
    }

    @Override
    public List<UsedDevicePromo> findAllByUid(String uid) {
        List<UsedDevicePromo> result = namedParameterJdbcTemplate.query(
                "select * from UsedDevicePromo where uid = :uid and provider in (:providers)",
                Map.of(
                        "uid",
                        Long.valueOf(uid),
                        "providers",
                        Stream.of(PromoProvider.values()).map(PromoProvider::name).collect(Collectors.toList())),
                rowMapper
        );

        return result;
    }

    private static class PromoStatisticsRowMapper implements RowMapper<PromoStatistics> {

        @Override
        public PromoStatistics mapRow(ResultSet rs, int rowNum) throws SQLException {
            return PromoStatistics.builder()
                    .promoType(rs.getString("promotype"))
                    .provider(rs.getString("provider"))
                    .platform(rs.getString("platform"))
                    .prototypeId(rs.getObject("prototype_id") == null ? null : rs.getInt("prototype_id"))
                    .leftCount(rs.getLong("left_count"))
                    .totalCount(rs.getLong("total_count"))
                    .build();
        }
    }
}
