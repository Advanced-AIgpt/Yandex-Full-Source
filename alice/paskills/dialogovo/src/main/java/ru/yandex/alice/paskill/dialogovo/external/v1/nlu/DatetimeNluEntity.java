package ru.yandex.alice.paskill.dialogovo.external.v1.nlu;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.Getter;

@EqualsAndHashCode(callSuper = true)
public class DatetimeNluEntity extends NluEntity {
    @Getter
    private final Value value;

    public DatetimeNluEntity(int begin, int end, Value value) {
        super(begin, end, BuiltinNluEntityType.DATETIME);
        this.value = value;
    }

    @Override
    public NluEntity withoutAdditionalValues() {
        return new DatetimeNluEntity(getTokens().getStart(), getTokens().getEnd(), value);
    }

    @Data
    @JsonInclude(JsonInclude.Include.NON_ABSENT)
    public static class Value {
        @Nullable
        private final Integer year;
        @Nullable
        private final Integer month;
        @Nullable
        private final Integer day;
        @Nullable
        private final Integer hour;
        @Nullable
        private final Integer minute;
        @Nullable
        @JsonProperty(value = "year_is_relative")
        private final Boolean yearIsRelative;
        @Nullable
        @JsonProperty(value = "month_is_relative")
        private final Boolean monthIsRelative;
        @Nullable
        @JsonProperty(value = "day_is_relative")
        private final Boolean dayIsRelative;
        @Nullable
        @JsonProperty(value = "hour_is_relative")
        private final Boolean hourIsRelative;
        @Nullable
        @JsonProperty(value = "minute_is_relative")
        private final Boolean minuteIsRelative;

        @Nullable
        public Boolean getYearIsRelative() {
            return year != null ? (yearIsRelative != null ? yearIsRelative : false) : null;
        }

        @Nullable
        public Boolean getMonthIsRelative() {
            return month != null ? (monthIsRelative != null ? monthIsRelative : false) : null;
        }

        @Nullable
        public Boolean getDayIsRelative() {
            return day != null ? (dayIsRelative != null ? dayIsRelative : false) : null;
        }

        @Nullable
        public Boolean getHourIsRelative() {
            return hour != null ? (hourIsRelative != null ? hourIsRelative : false) : null;
        }

        @Nullable
        public Boolean getMinuteIsRelative() {
            return minute != null ? (minuteIsRelative != null ? minuteIsRelative : false) : null;
        }
    }
}
