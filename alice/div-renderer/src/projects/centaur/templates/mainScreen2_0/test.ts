import MainScreen2_0 from './index';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { NAlice } from '../../../../protos';
import { getS3Asset } from '../../helpers/assets';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { createRequestState } from '../../../../registries/common';

const TColumn = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn;
const TCardData = NAlice.NData.TCentaurMainScreenMyScreenData.TColumn.TCardData;

jest.mock('../../../../common/logger');

const mmRequest = new MMRequest({}, {}, {});

describe('Main screen', () => {
    it('check snapshot without columns', () => {
        expect(AnonymizeDataForSnapshot(MainScreen2_0({
            Columns: [],
        }, mmRequest, createRequestState()))).toMatchSnapshot();
    });

    it('check snapshot without rows', () => {
        expect(AnonymizeDataForSnapshot(MainScreen2_0({
            Columns: [
                new TColumn({
                    Cards: [],
                }),
                new TColumn({
                    Cards: [],
                }),
                new TColumn({
                    Cards: [],
                }),
            ],
        }, mmRequest, createRequestState()))).toMatchSnapshot();
    });

    it('check snapshot with example data', () => {
        expect(AnonymizeDataForSnapshot(MainScreen2_0({
            Columns: [
                new TColumn({
                    Cards: [
                        new TCardData({
                            MusicCardData: {
                                Name: 'Плейлист дня',
                                Modified: {
                                    value: (new Date()).toString(),
                                },
                                Cover: '1',
                                Color: '#31AD54',
                            },
                        }),
                    ],
                }),
                new TColumn({
                    Cards: [
                        new TCardData({
                            InfoCardData: {
                                Title: 'test',
                                Description: 'Алиса, включи шум морской волны',
                                ImageBackground: getS3Asset('discovery/motivation.png'),
                                Color: '#3478C2',
                            },
                        }),
                        new TCardData({
                            WeatherCardData: {
                                Temperature: -3,
                                City: 'Москва',
                                Image: '1',
                                Comment: 'Пасмурно.\nОщущается как -5°',
                            },
                        }),
                    ],
                }),
                new TColumn({
                    Cards: [
                        new TCardData({
                            YouTubeCardData: {},
                        }),
                        new TCardData({
                            NewsCardData: {
                                Title: 'Медуза',
                                Content: 'Tesla начала устанавливать терминалы Starlink на зарядных станциях Supercharger',
                            },
                        }),
                    ],
                }),
            ],
        }, mmRequest, createRequestState()))).toMatchSnapshot();
    });
});
