import InfoCard from './InfoCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import { getInfoCardData } from './getData';
import { NAlice } from '../../../../../../protos';
import { logger } from '../../../../../../common/logger';
import { getS3Asset } from '../../../../helpers/assets';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getInfoCardData(new TCardData({}))).toBeNull();

        expect(getInfoCardData(new TCardData({
            YouTubeCardData: {},
        }))).toBeNull();

        expect(getInfoCardData(new TCardData({
            YouTubeCardData: {},
            InfoCardData: {
            },
        }))).not.toBeNull();

        expect(getInfoCardData(new TCardData({
            InfoCardData: {
                Title: '',
                Description: '',
                Icon: '',
                Subcomment: '',
                Color: '',
                ImageBackground: '',
            },
        }))).not.toBeNull();

        expect(getInfoCardData(new TCardData({
            InfoCardData: {
                Title: 'qwe',
                Description: 'qwe',
                Icon: 'qwe',
                Subcomment: 'qwe',
                Color: 'qwe',
                ImageBackground: 'qwe',
            },
        }))).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(InfoCard({
            type: 'info',
            color: '#3478C2',
            title: 'Расслабиться',
            image_background: getS3Asset('discovery/motivation.png'),
            description: 'Алиса, включи шум морской волны',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });

    it('should match snapshot with error', () => {
        expect(AnonymizeDataForSnapshot(InfoCard({
            type: 'info',
            color: '#3478C2',
            title: '',
            image_background: getS3Asset('discovery/motivation.png'),
            description: 'Алиса, включи шум морской волны',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalled();
    });
});
