import { ITrafficForecast } from './types';

export function sortTimeOfDay(originalArray: Readonly<ITrafficForecast[]>): ITrafficForecast[] {
    const forecastsFirst = [];
    const forecastsLast = [];

    for (const forecastElement of originalArray) {
        if (forecastElement.time === null || typeof forecastElement.time === 'undefined') {
            continue;
        }

        if (forecastsFirst.length === 0) {
            forecastsFirst.push(forecastElement);
        } else if (forecastsLast.length > 0) {
            forecastsLast.push(forecastElement);
        } else if (forecastElement.time - (forecastsFirst[forecastsFirst.length - 1].time as number) === 1) {
            forecastsFirst.push(forecastElement);
        } else {
            forecastsLast.push(forecastElement);
        }
    }

    return [...forecastsLast, ...forecastsFirst];
}
