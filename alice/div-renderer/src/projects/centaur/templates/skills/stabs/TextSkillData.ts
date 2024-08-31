import { NAlice } from '../../../../../protos';
type ITDialogovoSkillCardData = NAlice.NData.ITDialogovoSkillCardData;

export const params: ITDialogovoSkillCardData = {
    SkillInfo: {
        Name: 'Мистический Петербург',
        Logo: 'https://avatars.mds.yandex.net/get-dialogs/998463/ce24f9c3c26fd8375668/orig',
    },
    SkillRequest: {
        Text: 'начать квест',
    },
    SkillResponse: {
        TextResponse: {
            Text: 'Для начала квеста, уточните пожалуйста сколько',
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
