import YouTubeCard from './YouTubeCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import { logger } from '../../../../../../common/logger';
import { NAlice } from '../../../../../../protos';
import { getYouTubeCardData } from './getData';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getYouTubeCardData(new TCardData({}))).toBeNull();

        expect(getYouTubeCardData(new TCardData({
            NewsCardData: {},
        }))).toBeNull();

        expect(getYouTubeCardData(new TCardData({
            YouTubeCardData: {},
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(YouTubeCard({
            type: 'youtube',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });
});
