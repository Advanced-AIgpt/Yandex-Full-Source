import { galleryRender } from './gallery';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { NAlice } from '../../../../protos';

jest.mock('../../../../common/logger');

describe('Video scenario', () => {
    it('matches snapshot', () => {
        const scenarioData: NAlice.NData.ITGalleryData = {
            GalleryId: 'test',
            GalleryTitle: 'Test title',
            GalleryPosition: 0,
            GalleryParentScreen: 'main',
            RequestId: 'reqtst',
            ApphostRequestId: 'apphostreqtst',
            Cards: [
                {
                    ContentId: 'tstcontentid1',
                    ContentType: 'vod-episode',
                    TitleText: 'Тестовый фильм 1',
                    DescriptionText: 'Тестовое описание фильма 1',
                    Rating: 7.5678,
                    LegalLogoAlpha: 0,
                    LogoAlpha: 0,
                    OntoId: 'ruw123',
                    Genres: ['мелодрама', 'детектив'],
                    Countries: 'Канада',
                    ReleaseYear: 2011,
                    LogoUrls: {
                        BaseUrl: 'http://avatars.mds.yandex.net/get-ott/1672343/2a0000017af85caf7cd82a61b509216ee0e2/',
                        Sizes: ['120x90', '400x300', '1920x1080', 'orig'],
                    },
                },
                {
                    ContentId: 'tstcontentid2',
                    ContentType: 'vod-library',
                    TitleText: 'Тестовый фильм 2',
                    DescriptionText: 'Тестовое описание фильма 2',
                    RatingColor: '#123123',
                    Rating: 8.1,
                    LegalLogoAlpha: 0,
                    LogoAlpha: 0,
                    OntoId: 'ruw456',
                    Genres: ['триллер', 'драма'],
                    Countries: 'США',
                    ReleaseYear: 2021,
                    LogoUrls: {
                        BaseUrl: 'http://avatars.mds.yandex.net/get-ott/374297/2a0000017c180dfee9f6c1ee626b004923a1/',
                        Sizes: ['120x90', '400x300', '1920x1080', 'orig'],
                    },
                    LegalLogoUrls: {
                        BaseUrl: 'https://avatars.mds.yandex.net/get-ott/2419418/2a0000017eb9b60b5f8bd4d200217535fe5e/',
                        Sizes: ['orig'],
                    },
                },
            ],
        };
        expect(AnonymizeDataForSnapshot(galleryRender(scenarioData))).toMatchSnapshot();
    });
});
