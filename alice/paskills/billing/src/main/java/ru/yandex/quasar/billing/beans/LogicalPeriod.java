package ru.yandex.quasar.billing.beans;

import java.time.format.DateTimeParseException;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * ISO-8061 duration representation
 */
public final class LogicalPeriod {

    public static final LogicalPeriod ZERO = new LogicalPeriod(0, 0, 0, 0, 0, 0);
    /**
     * The pattern for parsing.
     */
    private static final Pattern PATTERN =
            Pattern.compile("([-+]?)P(?:([-+]?[0-9]+)Y)?(?:([-+]?[0-9]+)M)?(?:([-+]?[0-9]+)W)?(?:([-+]?[0-9]+)D)?" +
                    "(T(?:([-+]?[0-9]+)H)?(?:([-+]?[0-9]+)M)?(?:([-+]?[0-9]+)?S)?)?", Pattern.CASE_INSENSITIVE);

    /**
     * The number of years.
     */
    private final int years;
    /**
     * The number of months.
     */
    private final int months;
    /**
     * The number of days.
     */
    private final int days;

    /**
     * The number of hours.
     */
    private final int hours;

    /**
     * The number of minutes
     */
    private final int minutes;

    /**
     * The number of seconds
     */
    private final int seconds;


    //-----------------------------------------------------------------------

    /**
     * Constructor.
     *
     * @param years   the amount
     * @param months  the amount
     * @param days    the amount
     * @param hours   the amount
     * @param minutes the amount
     * @param seconds the amount
     */
    private LogicalPeriod(int years, int months, int days, int hours, int minutes, int seconds) {
        this.years = years;
        this.months = months;
        this.days = days;
        this.hours = hours;
        this.minutes = minutes;
        this.seconds = seconds;
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of years.
     * <p>
     * The resulting period will have the specified years.
     * The months and days units will be zero.
     *
     * @param years the number of years, positive or negative
     * @return the period of years, not null
     */
    public static LogicalPeriod ofYears(int years) {
        return create(years, 0, 0);
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of months.
     * <p>
     * The resulting period will have the specified months.
     * The years and days units will be zero.
     *
     * @param months the number of months, positive or negative
     * @return the period of months, not null
     */
    public static LogicalPeriod ofMonths(int months) {
        return create(0, months, 0);
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of weeks.
     * <p>
     * The resulting period will be day-based, with the amount of days
     * equal to the number of weeks multiplied by 7.
     * The years and months units will be zero.
     *
     * @param weeks the number of weeks, positive or negative
     * @return the period, with the input weeks converted to days, not null
     */
    public static LogicalPeriod ofWeeks(int weeks) {
        return create(0, 0, Math.multiplyExact(weeks, 7));
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of days.
     * <p>
     * The resulting period will have the specified days.
     * The years and months units will be zero.
     *
     * @param days the number of days, positive or negative
     * @return the period of days, not null
     */
    public static LogicalPeriod ofDays(int days) {
        return create(0, 0, days);
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of standard hours.
     * <p>
     * The seconds are calculated based on the standard definition of an hour,
     * where each hour is 3600 seconds.
     * The nanosecond in second field is set to zero.
     *
     * @param hours the number of hours, positive or negative
     * @return a {@code LogicalPeriod}, not null
     */
    public static LogicalPeriod ofHours(int hours) {
        return create(0, 0, 0, hours, 0, 0);
    }

    //-----------------------------------------------------------------------

    /**
     * Obtains a {@code LogicalPeriod} representing a number of standard minutes.
     * <p>
     * The seconds are calculated based on the standard definition of a minute,
     * where each minute is 60 seconds.
     * The nanosecond in second field is set to zero.
     *
     * @param minutes the number of minutes, positive or negative
     * @return a {@code LogicalPeriod}, not null
     */
    public static LogicalPeriod ofMinutes(int minutes) {
        return create(0, 0, 0, 0, minutes, 0);
    }

    //-----------------------------------------------------------------------

    /**
     * Obtains a {@code LogicalPeriod} representing a number of seconds.
     * <p>
     * The nanosecond in second field is set to zero.
     *
     * @param seconds the number of seconds, positive or negative
     * @return a {@code LogicalPeriod}, not null
     */
    public static LogicalPeriod ofSeconds(int seconds) {
        return create(0, 0, 0, 0, 0, seconds);
    }

    /**
     * Obtains a {@code LogicalPeriod} representing a number of years, months and days.
     * <p>
     * This creates an instance based on years, months and days.
     *
     * @param years  the amount of years, may be negative
     * @param months the amount of months, may be negative
     * @param days   the amount of days, may be negative
     * @return the period of years, months and days, not null
     */
    public static LogicalPeriod of(int years, int months, int days) {
        return create(years, months, days);
    }


    //-----------------------------------------------------------------------

    /**
     * Obtains a {@code LogicalPeriod} representing a number of years, months and days.
     * <p>
     * This creates an instance based on years, months and days.
     *
     * @param years   the amount of years, may be negative
     * @param months  the amount of months, may be negative
     * @param days    the amount of days, may be negative
     * @param hours   the amount of hours, may be negative
     * @param minutes the amount of minutes, may be negative
     * @param seconds the amount of seconds, may be negative
     * @return the period of years, months and days, not null
     */
    public static LogicalPeriod of(int years, int months, int days, int hours, int minutes, int seconds) {
        return create(years, months, days, hours, minutes, seconds);
    }

    /**
     * Obtains a {@code LogicalPeriod} from a text string such as {@code PnYnMnDTnHnMnS}.
     * <p>
     * This will parse the string produced by {@code toString()} which is
     * based on the ISO-8601 period formats {@code PnYnMnDTnHnMnS} and {@code PnW}.
     * <p>
     * The string starts with an optional sign, denoted by the ASCII negative
     * or positive symbol. If negative, the whole period is negated.
     * The ASCII letter "P" is next in upper or lower case.
     * The string starts with an optional sign, denoted by the ASCII negative
     * or positive symbol. If negative, the whole period is negated.
     * <p>
     * The leading plus/minus sign, and negative values for other units are
     * not part of the ISO-8601 standard. In addition, ISO-8601 does not
     * permit mixing between the {@code PnYnMnD} and {@code PnW} formats.
     * Any week-based input is multiplied by 7 and treated as a number of days.
     * <p>
     * For example, the following are valid inputs:
     * <pre>
     *   "P2Y"             -- LogicalPeriod.ofYears(2)
     *   "P3M"             -- LogicalPeriod.ofMonths(3)
     *   "P4W"             -- LogicalPeriod.ofWeeks(4)
     *   "P5D"             -- LogicalPeriod.ofDays(5)
     *   "PT15M"           -- LogicalPeriod.ofMinutes(15)
     *   "P1Y2M3D"         -- LogicalPeriod.of(1, 2, 3)
     *   "P1Y2M3W4D"       -- LogicalPeriod.of(1, 2, 25)
     *   "P-1Y2M"          -- LogicalPeriod.of(-1, 2, 0)
     *   "-P1Y2M"          -- LogicalPeriod.of(-1, -2, 0)
     *   "P1Y2M3DT4H5M6S"  -- LogicalPeriod.of(1, 2, 3, 4, 5, 6)
     * </pre>
     *
     * @param text the text to parse, not null
     * @return the parsed period, not null
     * @throws DateTimeParseException if the text cannot be parsed to a period
     */
    public static LogicalPeriod parse(CharSequence text) {
        Objects.requireNonNull(text, "text");
        Matcher matcher = PATTERN.matcher(text);
        if (matcher.matches()) {
            int negate = (charMatch(text, matcher.start(1), matcher.end(1), '-') ? -1 : 1);
            int yearStart = matcher.start(2);
            int yearEnd = matcher.end(2);
            int monthStart = matcher.start(3);
            int monthEnd = matcher.end(3);
            int weekStart = matcher.start(4);
            int weekEnd = matcher.end(4);
            int dayStart = matcher.start(5);
            int dayEnd = matcher.end(5);
            int hourStart = matcher.start(7);
            int hourEnd = matcher.end(7);
            int minuteStart = matcher.start(8);
            int minuteEnd = matcher.end(8);
            int secondStart = matcher.start(9);
            int secondEnd = matcher.end(9);
            if (yearStart >= 0 || monthStart >= 0 || weekStart >= 0 || dayStart >= 0 ||
                    hourStart >= 0 || minuteStart >= 0 || secondStart >= 0) {
                try {
                    int years = parseNumber(text, yearStart, yearEnd, negate);
                    int months = parseNumber(text, monthStart, monthEnd, negate);
                    int weeks = parseNumber(text, weekStart, weekEnd, negate);
                    int days = parseNumber(text, dayStart, dayEnd, negate);
                    days = Math.addExact(days, Math.multiplyExact(weeks, 7));

                    int hours = parseNumber(text, hourStart, hourEnd, negate);
                    int minutes = parseNumber(text, minuteStart, minuteEnd, negate);
                    int seconds = parseNumber(text, secondStart, secondEnd, negate);
                    return create(years, months, days, hours, minutes, seconds);
                } catch (NumberFormatException ex) {
                    throw new DateTimeParseException("Text cannot be parsed to a LogicalPeriod", text, 0, ex);
                }
            }
        }
        throw new DateTimeParseException("Text cannot be parsed to a LogicalPeriod", text, 0);
    }

    private static boolean charMatch(CharSequence text, int start, int end, char c) {
        return (start >= 0 && end == start + 1 && text.charAt(start) == c);
    }

    //-----------------------------------------------------------------------

    private static int parseNumber(CharSequence text, int start, int end, int negate) {
        if (start < 0 || end < 0) {
            return 0;
        }
        int val = Integer.parseInt(text, start, end, 10);
        try {
            return Math.multiplyExact(val, negate);
        } catch (ArithmeticException ex) {
            throw new DateTimeParseException("Text cannot be parsed to a LogicalPeriod", text, 0, ex);
        }
    }

    /**
     * Creates an instance.
     *
     * @param years  the amount
     * @param months the amount
     * @param days   the amount
     */
    private static LogicalPeriod create(int years, int months, int days) {
        if ((years | months | days) == 0) {
            return ZERO;
        }
        return new LogicalPeriod(years, months, days, 0, 0, 0);
    }

    /**
     * @param years   the amount
     * @param months  the amount
     * @param days    the amount
     * @param hours   the amount
     * @param minutes the amount
     * @param seconds the amount
     */
    private static LogicalPeriod create(int years, int months, int days, int hours, int minutes, int seconds) {
        if ((years | months | days | hours | minutes | seconds) == 0) {
            return ZERO;
        }
        return new LogicalPeriod(years, months, days, hours, minutes, seconds);
    }

    //-----------------------------------------------------------------------

    /**
     * Checks if all three units of this period are zero.
     * <p>
     * A zero period has the value zero for the years, months and days units.
     *
     * @return true if this period is zero-length
     */
    public boolean isZero() {
        return (this == ZERO);
    }

    /**
     * Checks if any of the three units of this period are negative.
     * <p>
     * This checks whether the years, months or days units are less than zero.
     *
     * @return true if any unit of this period is negative
     */
    public boolean isNegative() {
        return years < 0 || months < 0 || days < 0 || hours < 0 || minutes < 0 || seconds < 0;
    }

    //-----------------------------------------------------------------------

    /**
     * Gets the amount of years of this period.
     * <p>
     * This returns the years unit.
     * <p>
     * The months unit is not automatically normalized with the years unit.
     * This means that a period of "15 months" is different to a period
     * of "1 year and 3 months".
     *
     * @return the amount of years of this period, may be negative
     */
    public int getYears() {
        return years;
    }

    /**
     * Gets the amount of months of this period.
     * <p>
     * This returns the months unit.
     * <p>
     * The months unit is not automatically normalized with the years unit.
     * This means that a period of "15 months" is different to a period
     * of "1 year and 3 months".
     *
     * @return the amount of months of this period, may be negative
     */
    public int getMonths() {
        return months;
    }

    /**
     * Gets the amount of days of this period.
     * <p>
     * This returns the days unit.
     *
     * @return the amount of days of this period, may be negative
     */
    public int getDays() {
        return days;
    }

    /**
     * Gets the amount of days of this period.
     * <p>
     * This returns the days unit.
     *
     * @return the amount of days of this period, may be negative
     */
    public int getHours() {
        return hours;
    }

    public int getMinutes() {
        return minutes;
    }

    public int getSeconds() {
        return seconds;
    }

    /**
     * Gets the total number of months in this period.
     * <p>
     * This returns the total number of months in the period by multiplying the
     * number of years by 12 and adding the number of months.
     * <p>
     * This instance is immutable and unaffected by this method call.
     *
     * @return the total number of months in the period, may be negative
     */
    public long toTotalMonths() {
        return years * 12L + months;  // no overflow
    }

    //-----------------------------------------------------------------------

    /**
     * Checks if this period is equal to another period.
     * <p>
     * The comparison is based on the type {@code Period} and each of the three amounts.
     * To be equal, the years, months and days units must be individually equal.
     * Note that this means that a period of "15 Months" is not equal to a period
     * of "1 Year and 3 Months".
     *
     * @param obj the object to check, null returns false
     * @return true if this is equal to the other period
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof LogicalPeriod) {
            LogicalPeriod other = (LogicalPeriod) obj;
            return years == other.years &&
                    months == other.months &&
                    days == other.days &&
                    hours == other.hours &&
                    minutes == other.minutes &&
                    seconds == other.seconds;
        }
        return false;
    }

    /**
     * A hash code for this period.
     *
     * @return a suitable hash code
     */
    @Override
    public int hashCode() {
        int result = years;
        result = 31 * result + months;
        result = 31 * result + days;
        result = 31 * result + hours;
        result = 31 * result + minutes;
        result = 31 * result + seconds;
        return result;
    }


    //-----------------------------------------------------------------------

    /**
     * Outputs this period as a {@code String}, such as {@code P6Y3M1D}.
     * <p>
     * The output will be in the ISO-8601 period format.
     * A zero period will be represented as zero days, 'P0D'.
     *
     * @return a string representation of this period, not null
     */
    @Override
    public String toString() {
        if (this == ZERO) {
            return "P0D";
        } else {
            StringBuilder buf = new StringBuilder();
            buf.append('P');
            if (years != 0) {
                buf.append(years).append('Y');
            }
            if (months != 0) {
                buf.append(months).append('M');
            }
            if (days != 0) {
                buf.append(days).append('D');
            }
            if ((hours | minutes | seconds) != 0) {
                buf.append("T");
                if (hours != 0) {
                    buf.append(hours).append("H");
                }
                if (minutes != 0) {
                    buf.append(minutes).append("M");
                }
                if (seconds != 0) {
                    buf.append(seconds).append("S");
                }
            }
            return buf.toString();
        }
    }

}
