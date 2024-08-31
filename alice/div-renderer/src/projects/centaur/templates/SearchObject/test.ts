import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import SearchObject from './SearchObject';

jest.mock('../../../../common/logger');

describe('Entity Object scenario', () => {
    it('matches snapshot', () => {
        const mmRequest = new MMRequest({}, {}, {});

        expect(
            AnonymizeDataForSnapshot(SearchObject(
                {
                    Text: 'Советский и российский государственный деятель, российский военный. Министр обороны Российской Федерации с 6 ноября 2012 года. Генерал армии. Герой Российской Федерации. Заслуженный спасатель Российской Федерации. Член Высшего совета партии «Единая Россия».',
                    Image: 'https://avatars.mds.yandex.net/get-entity_search/2048976/478239945/S120x120',
                    SearchUrl: 'https://yandex.ru/search/touch/?l10n=ru-RU&lr=213&query_source=alice&text=%D1%88%D0%BE%D0%B9%D0%B3%D1%83',
                    Url: 'http://ru.wikipedia.org/wiki/Шойгу, Сергей Кужугетович',
                    Hostname: 'http://ru.wikipedia.org',
                    Title: 'Сергей Шойгу',
                    GalleryImages: [
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=57077838b1e87db79dd481d75280393f-4935650-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 100,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=107e6431697f675bd48eb4fd076a739e-5849607-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 84,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=551fda7fd5dae006024e01de3c45abc3-5827319-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 84,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=f264702fa8f6e5bc4402a6a0e3801e8a-4034704-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 96,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=a9ab9de1ff86c2443ae52a6f87e0bbc9-5905309-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 92,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=f93726267a6c2762c0e45910d79e7a98-5259114-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 92,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=9cba55dc901856709675cfd321f962ec-5896417-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 100,
                            ThmbWOrig: 150,
                        },
                        {
                            ThmbHref: '//avatars.mds.yandex.net/i?id=6acac7464255affa370b67e7d7e0fe25-5485004-images-thumbs&ref=oo_serp',
                            ThmbHOrig: 84,
                            ThmbWOrig: 150,
                        },
                    ],
                },
                mmRequest,
            )),
        ).toMatchSnapshot();
    });
});
