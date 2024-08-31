import { NAlice } from '../../../../../protos';
type ITDialogovoSkillCardData = NAlice.NData.ITDialogovoSkillCardData;

export const params: ITDialogovoSkillCardData = {
    SkillInfo: {
        Name: 'Мистический Петербург',
        Logo: 'https://avatars.mds.yandex.net/get-dialogs/998463/ce24f9c3c26fd8375668/mobile-logo-x4',
    },
    SkillRequest: {
        Text: 'историческая мозаика',
    },
    SkillResponse: {
        BigImageResponse: {
            ImageItem: {
                ImageUrl: 'https://avatars.mds.yandex.net/get-dialogs-skill-card/937455/eed017c000a2aab9ed6f/one-x4',
                Title: 'Историческая мозаика (7+)',
                Description: 'Квест в формате экскурсионной прогулки проходит по центру Петербурга, от Екатерининского сада до Невского проспекта. Вы сможете посетить самые интересные места, а мои головоломки позволят погрузиться в историю города. Подойдет как для туристов, так и для жителей города.',
                Button: null,
            },
        },
        buttons: [
            {
                Text: 'Подробное описание',
                Url: 'dialog://text_command?query=Подробное описание',
                Payload: null,
            },
            {
                Text: 'Начать квест',
                Url: 'dialog://text_command?query=Начать квест',
                Payload: null,
            },
        ],
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
