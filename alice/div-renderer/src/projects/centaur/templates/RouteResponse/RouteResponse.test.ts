import RouteResponse from './RouteResponse';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { createRequestState } from '../../../../registries/common';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { NAlice } from '../../../../protos';

jest.mock('../../../../common/logger');

process.env.SECRET_MAPS_SIGNING_SECRET = 'SECRET_MAPS_SIGNING_SECRET';
process.env.SECRET_MAPS_API_KEY = 'SECRET_MAPS_API_KEY';

describe('Tests to render RouteResponse', () => {
    it('should success rendered', () => {
        const mmRequest = new MMRequest({}, {}, {});
        const requestState = createRequestState(mmRequest);

        expect(AnonymizeDataForSnapshot(RouteResponse({
            'From': {
                'ResolvedLocation': {
                    'Geo': {
                        'AddressLine': 'Россия, Москва, улица Раменки, 11к2',
                        'City': 'Москва',
                        'CityCases': {
                            'Dative': 'Москве',
                            'Genitive': 'Москвы',
                            'Nominative': 'Москва',
                            'Preposition': 'в',
                            'Prepositional': 'Москве',
                        },
                        'CityPrepcase': 'в Москве',
                        'Country': 'Россия',
                        'GeoId': 213,
                        'House': '11к2',
                        'InUserCity': true,
                        'Level': 'inside_city',
                        'Street': 'улица Раменки',
                    },
                    'GeoUri': 'https://yandex.ru/maps?ll=37.49692%2C55.691014&ol=geo&oll=37.49692%2C55.691014&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%A0%D0%B0%D0%BC%D0%B5%D0%BD%D0%BA%D0%B8%2C%2011%D0%BA2',
                    'Location': {
                        'Lat': 55.69101891,
                        'Lon': 37.49691963,
                    },
                },
            },
            'To': {
                'ResolvedLocation': {
                    'CompanyName': 'Перекрёсток',
                    'Geo': {
                        'AddressLine': 'Россия, Москва, улица Раменки, 3',
                        'City': 'Москва',
                        'CityCases': {
                            'Dative': 'Москве',
                            'Genitive': 'Москвы',
                            'Nominative': 'Москва',
                            'Preposition': 'в',
                            'Prepositional': 'Москве',
                        },
                        'CityPrepcase': 'в Москве',
                        'Country': 'Россия',
                        'GeoId': 213,
                        'House': '3',
                        'InUserCity': true,
                        'Level': 'inside_city',
                        'Street': 'улица Раменки',
                    },
                    'GeoUri': 'https://yandex.ru/maps?ll=37.495969%2C55.69376&oid=48690801923&ol=biz&text=%D0%BC%D0%B0%D0%B3%D0%B0%D0%B7%D0%B8%D0%BD',
                    'Hours': {
                        'CurrentStatus': 'open',
                        'Timezone': 'Europe/Moscow',
                    },
                    'Location': {
                        'Lat': 55.69376,
                        'Lon': 37.495969,
                    },
                    'Name': 'Перекрёсток',
                    'ObjectCatalogPhotosUri': 'https://yandex.ru/profile/48690801923?intent=photo&lr=213',
                    'ObjectCatalogReviewsUri': 'https://yandex.ru/profile/48690801923?intent=reviews&lr=213',
                    'ObjectCatalogUri': 'https://yandex.ru/profile/48690801923?lr=213',
                    'ObjectId': '48690801923',
                    'ObjectUri': 'https://yandex.ru/maps/org/perekryostok/48690801923',
                    'Phone': '8 (800) 200-95-55',
                    'PhoneUri': 'tel:88002009555',
                    'Url': 'https://www.perekrestok.ru/',
                },
            },
            'Routes': [
                {
                    'Type': NAlice.NData.TRoute.EType.PEDESTRIAN,
                    'Length': {
                        'Text': '430 м',
                        'Value': 426,
                    },
                    'Time': {
                        'Text': '5 мин.',
                        'Value': 307,
                    },
                    'MapsUri': 'https://yandex.ru/maps?rtext=55.69101891%2C37.49691963~55.69376%2C37.495969&rtt=pd',
                    'ImageUri': 'https://static-maps.yandex.ru/1.x/?l=map&size=320%2C221&scale=1&lang=ru_RU&bbox=37.562312793000004%2C55.72581983405496~37.58967786699998%2C55.73424949614372&cr=0&lg=0&pt=37.58830525%2C55.73385747%2Cya_ru~37.56355666%2C55.72620304%2Chome&pl=c%3A8822DDC0%2Cw%3A5%2C37.58830525%2C55.73385747%2C37.587728%2C55.733369%2C37.587178%2C55.733559%2C37.573668%2C55.729508%2C37.572045%2C55.730159%2C37.568386%2C55.728137%2C37.564088%2C55.726981%2C37.56355666%2C55.72620304&key=AA6ZbVsBAAAA2YNHLQMAL2cFhx3aYrPKXQkPU4lT7kOqHP4AAAAAAAAAAADbLWvqk1JZt7nG6dYdK1ou-V63oA%3D%3D',
                },
                {
                    'Type': NAlice.NData.TRoute.EType.CAR,
                    'JamsTime': {
                        'Text': '3 мин.',
                        'Value': 200.7098813,
                    },
                    'Length': {
                        'Text': '510 м',
                        'Value': 510.3857098,
                    },
                    'Time': {
                        'Text': '4 мин.',
                        'Value': 213.1625113,
                    },
                    'MapsUri': 'https://yandex.ru/maps?rtext=55.69101891%2C37.49691963~55.69376%2C37.495969&rtt=auto',
                    'ImageUri': 'https://static-maps.yandex.ru/1.x/?l=map%2Ctrf&size=320%2C221&scale=1&lang=ru_RU&bbox=37.56240109999998%2C55.72583787767431~37.589722900000005%2C55.734162895646364&cr=0&lg=0&pt=37.58830525%2C55.73385747%2Cya_ru~37.56355666%2C55.72620304%2Chome&pl=c%3A8822DDC0%2Cw%3A5%2C37.588408%2C55.733785%2C37.587688%2C55.733296%2C37.587195%2C55.733621%2C37.578457%2C55.731047%2C37.576358%2C55.732903%2C37.568431%2C55.728238%2C37.563643%2C55.726947%2C37.563807%2C55.726216&key=AA6ZbVsBAAAA2YNHLQMAL2cFhx3aYrPKXQkPU4lT7kOqHP4AAAAAAAAAAADbLWvqk1JZt7nG6dYdK1ou-V63oA%3D%3D',
                },
                {
                    'Type': NAlice.NData.TRoute.EType.PUBLIC_TRANSPORT,
                    'Length': {
                        'Text': '430 м',
                        'Value': 426,
                    },
                    'Time': {
                        'Text': '5 мин.',
                        'Value': 307,
                    },
                    'MapsUri': 'https://yandex.ru/maps?rtext=55.69101891%2C37.49691963~55.69376%2C37.495969&rtt=mt',
                    'ImageUri': 'https://static-maps.yandex.ru/1.x/?l=map%2Ctrf&size=320%2C221&scale=1&lang=ru_RU&bbox=37.56158594199377%2C55.72270111661723~37.59628121813083%2C55.73644624141723&cr=0&lg=0&pt=37.58830525%2C55.73385747%2Cya_ru~37.56355666%2C55.72620304%2Chome&pl=c%3A8822DDC0%2Cw%3A5%2C37.58830525%2C55.73385747%2C37.59046%2C55.734271%2C37.592003%2C55.733694%2C37.594704%2C55.735818%2C37.588735%2C55.731252%2C37.57677%2C55.725928%2C37.564124%2C55.723326%2C37.565752%2C55.723814%2C37.563163%2C55.725309%2C37.56355666%2C55.72620304&key=AA6ZbVsBAAAA2YNHLQMAL2cFhx3aYrPKXQkPU4lT7kOqHP4AAAAAAAAAAADbLWvqk1JZt7nG6dYdK1ou-V63oA%3D%3D',
                },
            ],
        }, mmRequest, requestState))).toMatchSnapshot();
    });

    it('should success rendered with empty routes', () => {
        const mmRequest = new MMRequest({}, {}, {});
        const requestState = createRequestState(mmRequest);

        expect(AnonymizeDataForSnapshot(RouteResponse({
            'From': {
                'ResolvedLocation': {
                    'Geo': {
                        'AddressLine': 'Россия, Москва, улица Раменки, 11к2',
                        'City': 'Москва',
                        'CityCases': {
                            'Dative': 'Москве',
                            'Genitive': 'Москвы',
                            'Nominative': 'Москва',
                            'Preposition': 'в',
                            'Prepositional': 'Москве',
                        },
                        'CityPrepcase': 'в Москве',
                        'Country': 'Россия',
                        'GeoId': 213,
                        'House': '11к2',
                        'InUserCity': true,
                        'Level': 'inside_city',
                        'Street': 'улица Раменки',
                    },
                    'GeoUri': 'https://yandex.ru/maps?ll=37.49692%2C55.691014&ol=geo&oll=37.49692%2C55.691014&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0%2C%20%D1%83%D0%BB%D0%B8%D1%86%D0%B0%20%D0%A0%D0%B0%D0%BC%D0%B5%D0%BD%D0%BA%D0%B8%2C%2011%D0%BA2',
                    'Location': {
                        'Lat': 55.69101891,
                        'Lon': 37.49691963,
                    },
                },
            },
            'To': {
                'ResolvedLocation': {
                    'Geo': {
                        'AddressLine': 'Мадагаскар, Антананариву',
                        'City': 'Антананариву',
                        'CityCases': {
                            'Dative': 'Антананариву',
                            'Genitive': 'Антананариву',
                            'Nominative': 'Антананариву',
                            'Preposition': 'в',
                            'Prepositional': 'Антананариву',
                        },
                        'CityPrepcase': 'в Антананариву',
                        'Country': 'Мадагаскар',
                        'GeoId': 20855,
                        'Level': 'city',
                    },
                    'Location': {
                        'Lat': -18.901408,
                        'Lon': 47.522172,
                    },
                },
            },
        }, mmRequest, requestState))).toMatchSnapshot();
    });
});
