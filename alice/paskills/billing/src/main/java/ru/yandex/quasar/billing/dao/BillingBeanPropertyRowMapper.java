package ru.yandex.quasar.billing.dao;

import java.beans.PropertyDescriptor;
import java.io.IOException;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.time.Instant;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.jdbc.core.BeanPropertyRowMapper;
import org.springframework.jdbc.support.JdbcUtils;

import ru.yandex.quasar.billing.beans.DeliveryInfo;
import ru.yandex.quasar.billing.beans.PricingOption;
import ru.yandex.quasar.billing.beans.ProviderContentItem;
import ru.yandex.quasar.billing.services.promo.Platform;

/**
 * A convenience {@link BeanPropertyRowMapper} that decodes a {@link ProviderContentItem} stored as string in text
 * fields.
 *
 * @param <T>
 */
class BillingBeanPropertyRowMapper<T> extends BeanPropertyRowMapper<T> {
    private final ObjectMapper objectMapper;

    BillingBeanPropertyRowMapper(Class<T> clazz, ObjectMapper objectMapper) {
        super(clazz);

        this.objectMapper = objectMapper;
    }

    @Override
    @Nullable
    protected Object getColumnValue(ResultSet rs, int index, PropertyDescriptor pd) throws SQLException {

        if (pd.getPropertyType() == ProviderContentItem.class) {
            String resultSetValue = (String) JdbcUtils.getResultSetValue(rs, index, String.class);
            try {
                return resultSetValue != null ? objectMapper.readValue(resultSetValue, ProviderContentItem.class) :
                        null;
            } catch (IOException e) {
                throw new SQLException(e);
            }
        } else if (pd.getPropertyType() == PricingOption.class) {
            String resultSetValue = (String) JdbcUtils.getResultSetValue(rs, index, String.class);
            try {
                return resultSetValue != null ?
                        objectMapper.readValue(
                                resultSetValue,
                                PricingOption.class
                        ) : null;
            } catch (IOException e) {
                throw new SQLException(e);
            }
        } else if (pd.getPropertyType() == DeliveryInfo.class) {
            String resultSetValue = (String) JdbcUtils.getResultSetValue(rs, index, String.class);
            try {
                return resultSetValue != null ?
                        objectMapper.readValue(
                                resultSetValue,
                                DeliveryInfo.class
                        ) : null;
            } catch (IOException e) {
                throw new SQLException(e);
            }
        } else if (pd.getPropertyType() == Platform.class) {
            String value = (String) JdbcUtils.getResultSetValue(rs, index, String.class);
            return value != null ? Platform.create(value) : null;
        } else if (pd.getPropertyType() == Instant.class) {
            Timestamp tsValue = (Timestamp) JdbcUtils.getResultSetValue(rs, index, Timestamp.class);
            return tsValue != null ? tsValue.toInstant() : null;
        } else {
            return super.getColumnValue(rs, index, pd);
        }
    }
}
