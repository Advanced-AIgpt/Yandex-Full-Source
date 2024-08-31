// import teasersRender from './teasers';
import scenarioRender from './scenario';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { createRequestState } from '../../../../registries/common';
import { ExpFlags } from '../../expFlags';

jest.mock('../../../../common/logger');

// TODO: вернуть тест когда с сервера начнет приходить server_time
// describe('News teaser', () => {
//     it('matches snapshot', () => {
//         expect(AnonymizeDataForSnapshot(teasersRender({
//             Topic: 'sport',
//             NewsItem: {
//                 Agency: 'agency',
//                 Url: 'https://yandex.ru/news/1',
//                 Image: {
//                     Src: 'https://avatars.mds.yandex.net/get-bass/image/60x60',
//                 },
//                 Text: 'text',
//                 PubDate: 1634130001,
//                 Logo: 'https://avatars.mds.yandex.net/get-ynews-logo/26056/1071-1582024400668-square/orig',
//             },
//             Tz: 'Europe/Moscow',
//         }))).toMatchSnapshot();
//     });
// });

describe('News scenario', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(scenarioRender({
            CurrentNewsItem: 0,
            NewsItems: [{
                Agency: 'agency',
                Image: {
                    Src: 'https://avatars.mds.yandex.net/get-bass/image/60x60',
                },
                Logo: 'imager',
                PubDate: 1000000,
                Messages: 'messages',
                Text: 'text',
                TurboIconUrl: 'https://avatars.mds.yandex.net/get-bass/turbo/32x32',
                Url: 'https://yandex.ru/news/1',
            }],
            Topic: 'world',
        }, new MMRequest({}, {}, {}), createRequestState()))).toMatchSnapshot();
    });
});

describe('News scenario with extended news', () => {
    const mmRequest = new MMRequest({}, {}, {
        [ExpFlags.extendedNewsDesignWithDoubleScreen as string]: '',
    });

    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(scenarioRender({
            CurrentNewsItem: 0,
            NewsItems: [{
                Agency: 'agency',
                Image: {
                    Src: 'https://avatars.mds.yandex.net/get-bass/image/60x60',
                },
                Logo: 'imager',
                PubDate: 1000000,
                Messages: 'messages',
                Text: 'text',
                TurboIconUrl: 'https://avatars.mds.yandex.net/get-bass/turbo/32x32',
                Url: 'https://yandex.ru/news/1',
            }],
            Topic: 'world',
        }, mmRequest, createRequestState(mmRequest)))).toMatchSnapshot();
    });
});
