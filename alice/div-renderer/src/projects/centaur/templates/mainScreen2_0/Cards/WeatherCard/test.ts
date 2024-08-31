import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import WeatherCard from './WeatherCard';
import { NAlice } from '../../../../../../protos';
import { getWeatherCardData } from './getData';
import { logger } from '../../../../../../common/logger';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getWeatherCardData(new TCardData({}))).toBeNull();

        expect(getWeatherCardData(new TCardData({
            YouTubeCardData: {},
        }))).toBeNull();

        expect(getWeatherCardData(new TCardData({
            YouTubeCardData: {},
            WeatherCardData: {
            },
        }))).not.toBeNull();

        expect(getWeatherCardData(new TCardData({
            WeatherCardData: {
                Temperature: -3,
                City: 'Москва',
                Image: '1',
                Comment: 'Пасмурно.\nОщущается как -5°',
            },
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(WeatherCard({
            type: 'weather',
            temperature: -3,
            city: 'Москва',
            image: '1',
            comment: 'Пасмурно.\nОщущается как -5°',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });

    it('should match snapshot with error', () => {
        expect(AnonymizeDataForSnapshot(WeatherCard({
            type: 'weather',
            temperature: undefined,
            city: '',
            image: '',
            comment: 'Пасмурно.\nОщущается как -5°',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalled();
    });
});
