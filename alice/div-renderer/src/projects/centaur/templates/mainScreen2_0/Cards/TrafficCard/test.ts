import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import TrafficCard from './TrafficCard';
import { NAlice } from '../../../../../../protos';
import { getTrafficCardData } from './getData';
import { logger } from '../../../../../../common/logger';

const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getTrafficCardData(new TCardData({}))).toBeNull();

        expect(getTrafficCardData(new TCardData({
            YouTubeCardData: {},
        }))).toBeNull();

        expect(getTrafficCardData(new TCardData({
            YouTubeCardData: {},
            TrafficCardData: {
            },
        }))).not.toBeNull();

        expect(getTrafficCardData(new TCardData({
            TrafficCardData: {
                Forecast: [],
                City: 'Москва',
                Level: 'level',
                Score: 7,
                Message: 'Облачно, без пробок',
            },
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(TrafficCard({
            type: 'traffic',
            text: 'Текст о пробках',
            city: 'Москва',
            trafficValue: 7,
            forecasts: [
                {
                    time: 17,
                    trafficValue: 8,
                },
            ],
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });

    it('should match snapshot with error', () => {
        expect(AnonymizeDataForSnapshot(TrafficCard({
            type: 'traffic',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalled();
    });
});
