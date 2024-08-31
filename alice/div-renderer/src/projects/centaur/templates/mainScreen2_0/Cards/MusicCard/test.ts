import MusicCard from './MusicCard';
import { AnonymizeDataForSnapshot } from '../../../../../../common/helpers/dev';
import { getMusicCardData } from './getData';
import { NAlice } from '../../../../../../protos';
import { logger } from '../../../../../../common/logger';
import { createRequestState } from '../../../../../../registries/common';
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../../../common/logger');

describe('Main screen empty card', () => {
    it('should get actual data', () => {
        expect(getMusicCardData(new TCardData({}), createRequestState())).toBeNull();

        expect(getMusicCardData(new TCardData({
            YouTubeCardData: {},
        }), createRequestState())).toBeNull();

        expect(getMusicCardData(new TCardData({
            YouTubeCardData: {},
            MusicCardData: {
            },
        }), createRequestState())).not.toBeNull();

        expect(getMusicCardData(new TCardData({
            MusicCardData: {
                Name: 'Плейлист дня',
                Modified: {
                    value: 'Обновлен сегодня',
                },
                Cover: '1',
                Color: '#31AD54',
            },
        }), createRequestState())).not.toBeNull();
    });

    it('should match snapshot', () => {
        expect(AnonymizeDataForSnapshot(MusicCard({
            type: 'music',
            name: 'Плейлист дня',
            color: '#31AD54',
            description: 'Обновлен сегодня',
            cover: 'link_to_image',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalledTimes(0);
    });

    it('should match snapshot with error', () => {
        expect(AnonymizeDataForSnapshot(MusicCard({
            type: 'music',
            name: '',
            color: '#31AD54',
            description: 'Обновлен сегодня',
            cover: 'link_to_image',
        }))).toMatchSnapshot();

        expect(logger.error).toHaveBeenCalled();
    });
});
