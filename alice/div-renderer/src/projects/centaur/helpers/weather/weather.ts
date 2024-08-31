export function getTemperatureString(temperature: number): string {
    const temperatureValue = Math.abs(temperature);
    let prefix = '';

    if (temperature > 0) {
        prefix = '+';
    } else if (temperature < 0) {
        prefix = '–';
    }

    return `${prefix}${temperatureValue}°`;
}
