import { renderDiscoveryTab } from './discovery';
import { AnonymizeDataForSnapshot } from '../../../../common/helpers/dev';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { MainScreenMusicTab } from '../MainScreenMusicTab/MainScreenMusicTab';
import { createRequestState } from '../../../../registries/common';

jest.mock('../../../../common/logger');

describe('Main screen - discovery', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(renderDiscoveryTab({}))).toMatchSnapshot();
    });
});

describe('Main screen - music', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(MainScreenMusicTab({
            Id: 'music.gallery.tab.block',
            HorizontalMusicBlockData: [{
                Type: 'inf_feed_play_contexts',
                Title: 'Вы недавно слушали',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '164354',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/108289/679d0011.p.164354/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_164354',
                    Type: 'artist',
                    Title: 'Артист',
                    Genres: ['Классика'],
                }],
            }, {
                Type: 'inf_feed_liked_artists',
                Title: 'Ваши любимые исполнители',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '164354',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/108289/679d0011.p.164354/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_164354',
                    Type: 'artist',
                    Title: 'Артист',
                    Genres: ['Классика'],
                }],
            }, {
                Type: 'inf_feed_user_library_playlists',
                Title: 'Ваши плейлисты',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '3',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/28719/291769700.1017.3715/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_3',
                    Type: 'playlist',
                    Title: 'Мне нравится',
                    Modified: { value: '2021-11-01T23:01:23+00:00' },
                    LikesCount: {},
                }, {
                    Id: '1000',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/28719/1499372368.1000.15968/460x460?1635807579564',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1000',
                    Type: 'playlist',
                    Title: 'Созданный плейлист',
                    Modified: { value: '2021-11-01T22:58:14+00:00' },
                    LikesCount: {},
                }],
            }, {
                Type: 'inf_feed_liked_podcasts',
                Title: 'Ваши любимые подкасты',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '18837614',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5496390/7e294c2d.a.18837614-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_18837614',
                    Type: 'album',
                    Title: 'Время и деньги',
                    LikesCount: { value: 3076 },
                    ReleaseDate: {},
                }],
            }, {
                Type: 'inf_feed_recommended_podcasts',
                Title: 'Подкасты',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '18837614',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5496390/7e294c2d.a.18837614-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_18837614',
                    Type: 'album',
                    Title: 'Время и деньги',
                    LikesCount: { value: 3076 },
                    ReleaseDate: {},
                }, {
                    Id: '19096372',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5496390/b07791b6.a.19096372-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_19096372',
                    Type: 'album',
                    Title: 'Фрэнк Герберт. «Дети Дюны»',
                    LikesCount: { value: 154 },
                    ReleaseDate: { value: '2021-11-01T00:00:00+03:00' },
                    Artists: [{ Id: '7367533', Name: 'Сергей Чонишвили' }],
                }, {
                    Id: '14029755',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5234847/388d0491.a.14029755-2/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_14029755',
                    Type: 'album',
                    Title: '7 ВОПРОСОВ СУПЕРЗВЕЗДЕ!',
                    LikesCount: { value: 1784 },
                    ReleaseDate: {},
                }, {
                    Id: '18154810',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4399834/582b0ff8.a.18154810-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_18154810',
                    Type: 'album',
                    Title: 'Даниель Оберг. «Вирус»',
                    LikesCount: { value: 1754 },
                    ReleaseDate: { value: '2021-09-15T00:00:00+03:00' },
                    Artists: [{ Id: '13219484', Name: 'Наталья Русинова' }],
                }, {
                    Id: '17557011',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4399834/95d9a3a8.a.17557011-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_17557011',
                    Type: 'album',
                    Title: 'Научи меня плохому',
                    LikesCount: { value: 3606 },
                    ReleaseDate: {},
                }, {
                    Id: '18153936',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5236179/b866e01f.a.18153936-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_18153936',
                    Type: 'album',
                    Title: 'Дмитрий Глуховский. «Пост»',
                    LikesCount: { value: 12867 },
                    ReleaseDate: { value: '2021-09-15T00:00:00+03:00' },
                    Artists: [{ Id: '13219074', Name: 'Дмитрий Глуховский' }],
                }, {
                    Id: '8445628',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4406810/f4cd59ae.a.8445628-2/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_8445628',
                    Type: 'album',
                    Title: 'Жуть',
                    LikesCount: { value: 18179 },
                    ReleaseDate: {},
                }, {
                    Id: '18743840',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5457712/4c12439f.a.18743840-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_18743840',
                    Type: 'album',
                    Title: 'Харламов читает Хармса',
                    LikesCount: { value: 1603 },
                    ReleaseDate: {},
                }, {
                    Id: '8770286',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/139444/69ac9bac.a.8770286-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_8770286',
                    Type: 'album',
                    Title: 'Я боюсь',
                    LikesCount: { value: 6399 },
                    ReleaseDate: {},
                }, {
                    Id: '13260902',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/2357076/7dc6b30f.a.13260902-1/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_13260902',
                    Type: 'album',
                    Title: 'Спасибо, я в порядке',
                    LikesCount: { value: 17101 },
                    ReleaseDate: {},
                }],
            }, {
                Type: 'inf_feed_tag_playlists-for_kids',
                Title: 'Для детей',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '1364',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/51766/103372440.1364.3645/460x460?1590159303645',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1364',
                    Type: 'playlist',
                    Title: '«Союзмультфильм»: песни Чебурашки и других героев',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 15505 },
                }, {
                    Id: '2298',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/71140/103372440.2298.68152/460x460?1629487568152',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_2298',
                    Type: 'playlist',
                    Title: 'Весёлая зарядка',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 5379 },
                }, {
                    Id: '1087',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/59900/543081004.1087.59868ru/460x460?1611936860774',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1087',
                    Type: 'playlist',
                    Title: 'На репите: детские песни',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 912 },
                }, {
                    Id: '1048',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/69910/970829816.1048.19040/460x460?1629487019040',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1048',
                    Type: 'playlist',
                    Title: 'Потешки',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 4902 },
                }, {
                    Id: '2296',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/38125/103372440.2296.73397/460x460?1629125173397',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_2296',
                    Type: 'playlist',
                    Title: '«Фиксики»',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 10689 },
                }, {
                    Id: '1353',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/69910/103372440.1353.69746/460x460?1629487369746',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1353',
                    Type: 'playlist',
                    Title: 'Песни из советских мультфильмов',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 21962 },
                }, {
                    Id: '2262',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/28719/103372440.2262.48167/460x460?1590159448167',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_2262',
                    Type: 'playlist',
                    Title: 'Волшебная музыка для чтения сказок',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 26266 },
                }, {
                    Id: '2297',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/38125/103372440.2297.44368/460x460?1629125244368',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_2297',
                    Type: 'playlist',
                    Title: 'Поют Смешарики',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 9829 },
                }],
            }, {
                Type: 'inf_feed_tag_playlists-5ebeb05d501c1e482a6f926b',
                Title: 'Популярное',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '1029',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/30088/139954184.1029.26944ru/460x460?1635241427896',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1029',
                    Type: 'playlist',
                    Title: 'Хиты FM',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 417104 },
                }, {
                    Id: '1111',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/30088/103372440.1111.92350/460x460?1623918792350',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1111',
                    Type: 'playlist',
                    Title: 'В сердечке',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 94581 },
                }, {
                    Id: '1829',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/71140/103372440.1829.48443ru/460x460?1629441449014',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1829',
                    Type: 'playlist',
                    Title: 'Топ распознаваний',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 79455 },
                }, {
                    Id: '1234',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/38125/103372440.1234.49195ru/460x460?1633020150127',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1234',
                    Type: 'playlist',
                    Title: 'Новые хиты',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 51508 },
                }, {
                    Id: '1878',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/38125/103372440.1878.8252ru/460x460?1635282309678',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1878',
                    Type: 'playlist',
                    Title: 'За кадром',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 68257 },
                }, {
                    Id: '1102',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/38125/103372440.1102.7710ru/460x460?1634019608322',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1102',
                    Type: 'playlist',
                    Title: '100 хитов русской поп-музыки',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 45812 },
                }, {
                    Id: '1101',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/59900/103372440.1101.54603ru/460x460?1633342655745',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1101',
                    Type: 'playlist',
                    Title: '100 суперхитов',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 106556 },
                }, {
                    Id: '1104',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/71140/103372440.1104.12681ru/460x460?1618302813329',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1104',
                    Type: 'playlist',
                    Title: '100 хитов русского рэпа',
                    Modified: { value: '2021-11-01T23:08:47+00:00' },
                    LikesCount: { value: 18917 },
                }],
            }, {
                Type: 'inf_feed_popular_artists',
                Title: 'Популярные исполнители',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '816919',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4796762/37242f02.p.816919/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_816919',
                    Type: 'artist',
                    Title: 'Minelli',
                    Genres: ['Танцевальная', 'Поп', 'Электроника'],
                }, {
                    Id: '5129397',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4010467/46881e6e.p.5129397/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_5129397',
                    Type: 'artist',
                    Title: 'HammAli & Navai',
                    Genres: ['Русский рэп'],
                }, {
                    Id: '5007577',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4384958/35de05ab.p.5007577/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_5007577',
                    Type: 'artist',
                    Title: 'Zivert',
                    Genres: ['Русская поп-музыка'],
                }, {
                    Id: '666984',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5207413/fe940cc8.p.666984/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_666984',
                    Type: 'artist',
                    Title: 'Artik & Asti',
                    Genres: ['Русская поп-музыка'],
                }, {
                    Id: '3827925',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/2810397/a3d6b6b6.p.3827925/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_3827925',
                    Type: 'artist',
                    Title: 'Filatov & Karas',
                    Genres: ['Танцевальная'],
                }, {
                    Id: '4944372',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4404215/d0a00f99.p.4944372/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_4944372',
                    Type: 'artist',
                    Title: 'NILETTO',
                    Genres: ['Русская поп-музыка'],
                }, {
                    Id: '5056591',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/5314916/f7daebf9.p.5056591/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_5056591',
                    Type: 'artist',
                    Title: 'Джарахов',
                    Genres: ['Русский рэп'],
                }, {
                    Id: '1554548',
                    ImageUrl: 'https://avatars.yandex.net/get-music-content/4404215/d699608e.p.1554548/460x460',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_1554548',
                    Type: 'artist',
                    Title: 'Dabro',
                    Genres: ['Русская поп-музыка'],
                }],
            }, {
                Type: 'inf_feed_auto_playlists',
                Title: 'Собрано для вас',
                CentaurMainScreenGalleryMusicCardData: [{
                    Id: '145894115',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/30088/r1vptaVA3bMLcU/460x460?1635746159196',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_145894115',
                    Type: 'auto_playlist',
                    Title: 'Плейлист дня',
                    Modified: { value: '2021-11-01T22:31:36+00:00' },
                }, {
                    Id: '138075091',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/70586/qqu9n7yeaMJZNy/460x460?1617202772520',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_138075091',
                    Type: 'auto_playlist',
                    Title: 'Премьера',
                    Modified: { value: '2021-11-01T22:31:36+00:00' },
                }, {
                    Id: '38167075',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/51766/qquaac4OOuu8t8/460x460?1617203605659',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_38167075',
                    Type: 'auto_playlist',
                    Title: 'Плейлист с Алисой',
                    Modified: { value: '2021-11-01T22:31:36+00:00' },
                }, {
                    Id: '30978063',
                    ImageUrl: 'https://avatars.yandex.net/get-music-user-playlist/34120/qxb7vaPme5AfcK/460x460?1628071318873',
                    Action: '@@mm_deeplink#OnClickMainScreenMusicTab_30978063',
                    Type: 'auto_playlist',
                    Title: 'Подкасты',
                    Modified: { value: '2021-11-01T22:31:36+00:00' },
                }],
            }],
        }, new MMRequest({}, {}, {}), createRequestState()))).toMatchSnapshot();
    });
});
