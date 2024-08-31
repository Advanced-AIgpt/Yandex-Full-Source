export const formatTimeUnit = (value: number): string => value >= 10 ? String(value) : '0' + value;

const minuteInSeconds = 60;
const hourInSeconds = 60 * minuteInSeconds;

export const formatTime = (value: number) => {
    const hours = Math.floor(value / hourInSeconds);
    const secondsRemainder = value % hourInSeconds;
    const minutes = Math.floor(secondsRemainder / minuteInSeconds);
    const seconds = secondsRemainder % minuteInSeconds;

    const result: string[] = [];

    if (hours > 0) {
        result.push(formatTimeUnit(hours));
    }

    result.push(formatTimeUnit(minutes));

    result.push(formatTimeUnit(seconds));

    return result.join(':');
};
