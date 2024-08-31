import NewsCard from './NewsCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import { NAlice } from '../../../../../../protos';
import { getNewsCardData } from './getData';
import { logger } from '../../../../../../common/logger';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getNewsCardData(new TCardData({}))).toBeNull();

        expect(getNewsCardData(new TCardData({
            YouTubeCardData: {},
        }))).toBeNull();

        expect(getNewsCardData(new TCardData({
            YouTubeCardData: {},
            NewsCardData: {
            },
        }))).not.toBeNull();

        expect(getNewsCardData(new TCardData({
            NewsCardData: {
                Title: 'Медуза',
                Content: 'Tesla начала устанавливать терминалы Starlink на зарядных станциях Supercharger',
            },
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(NewsCard({
            type: 'news',
            title: 'Медуза',
            content: 'Tesla начала устанавливать терминалы Starlink на зарядных станциях Supercharger',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });
    it('should match snapshot with error', () => {
        expect(AnonymizeDataForSnapshot(NewsCard({
            type: 'news',
            title: '',
            content: 'Tesla начала устанавливать терминалы Starlink на зарядных станциях Supercharger',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalled();
    });
});
