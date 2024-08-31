import { NAlice } from '../../../../../protos';
type ITDialogovoSkillCardData = NAlice.NData.ITDialogovoSkillCardData;

export const params: ITDialogovoSkillCardData = {
    SkillInfo: {
        Name: 'Мистический Петербург',
        Logo: 'https://avatars.mds.yandex.net/get-dialogs/998463/ce24f9c3c26fd8375668/orig',
    },
    SkillRequest: {
        Text: 'квесты',
    },
    SkillResponse: {
        ItemsListResponse: {
            ItemsLisetHeader: {
                Text: null,
            },
            ImageItems: [
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'Историческая мозаика',
                    Description: 'беплатно, 1-10 чел., от 7 лет',
                    Button: {
                        Text: 'Историческая мозаика',
                        Url: 'dialog://text_command?query=Историческая мозаика',
                        Payload: null,
                    },
                },
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'По следам Пиковой Дамы',
                    Description: 'Цена: 3000 руб., 1-5 чел., от 7 лет',
                    Button: {
                        Text: 'По следам Пиковой Дамы',
                        Url: 'dialog://text_command?query=По следам Пиковой Дамы',
                        Payload: null,
                    },
                },
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'Безумие Алисы',
                    Description: 'Цена: 4000 руб., 2-7 чел., от 16 лет',
                    Button: {
                        Text: 'Безумие Алисы',
                        Url: 'dialog://text_command?query=Безумие Алисы',
                        Payload: null,
                    },
                },
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'Безумие Алисы',
                    Description: 'Цена: 4000 руб., 2-7 чел., от 16 лет',
                    Button: {
                        Text: 'Безумие Алисы',
                        Url: 'dialog://text_command?query=Безумие Алисы',
                        Payload: null,
                    },
                },
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'Безумие Алисы',
                    Description: 'Цена: 4000 руб., 2-7 чел., от 16 лет',
                    Button: {
                        Text: 'Безумие Алисы',
                        Url: 'dialog://text_command?query=Безумие Алисы',
                        Payload: null,
                    },
                },
                {
                    ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/213044/923f22cc2a912ddd68e4/orig',
                    Title: 'Безумие Алисы',
                    Description: 'Цена: 4000 руб., 2-7 чел., от 16 лет',
                    Button: {
                        Text: 'Безумие Алисы',
                        Url: 'dialog://text_command?query=Безумие Алисы',
                        Payload: null,
                    },
                },
            ],
            ItemsLisetFooter: {
                Text: null,
                Button: null,
            },
        },
        buttons: [],
        suggests: [
            {
                Text: 'Меню',
                Url: 'dialog://text_command?query=Меню',
                Payload: null,
            },
            {
                Text: 'Все квесты',
                Url: 'dialog://text_command?query=Все квесты',
                Payload: null,
            },
            {
                Text: 'Мои квесты',
                Url: 'dialog://text_command?query=Мои квесты',
                Payload: null,
            },
            {
                Text: 'Авторизация',
                Url: 'dialog://text_command?query=Авторизация',
                Payload: null,
            },
        ],
    },
};
