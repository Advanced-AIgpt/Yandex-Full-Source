import {AudioPlayerIntentNames, ImplicitDiscoveryIntentNames, IncomingMessage, Slots} from './transport';
import {wrapResponse} from './utils';
import {State} from './types';
import {handlePlayerEvent} from './handlers/player';
import {IncomingHttpHeaders} from 'http';
import {v4 as uuidv4} from 'uuid';
import {songsByToken} from './fixtures';
import moment from "moment";

export async function handleIncomingMessage(
    incomingMessage: IncomingMessage,
    headers: IncomingHttpHeaders,
): Promise<any> {
    // const userId = incomingMessage.session.user_id;

    //metacommand
    const message = incomingMessage as any;

    console.log('user_state', incomingMessage.state?.user);

    if (incomingMessage.request?.type == "Show.Pull") {
        let date = new Date();
        let publicationDate = moment(date).startOf('day');
        const currentDate = publicationDate.format("YY-MM-DD");
        return wrapResponse(incomingMessage, {
            text: "Сегодня у нас слово «freaking».\n\nПереводится как «долбаный» или «капец какой». Выручит, когда хочется сказать то самое нецензурное слово на букву «f», но нельзя. Это прилагательное поможет выразить любые эмоции — от возмущения и обиды до восторга.",
            tts: "Сегодня у нас слово «<speaker audio=\"dialogs-upload/47718ad7-ee3e-4e05-94ec-dbbd8e5c5cf7/612b61ca-be77-4e30-b746-946be84b6d83.opus\">».\n\nПереводится как «долбаный» или «капец какой». Выручит, когда хочется сказать то самое нецензурное слово на букву «f», но нельзя. Это прилагательное поможет выразить любые эмоции — от возмущения и обиды до восторга.",
            show_item_meta: {
                content_id: "uid " + currentDate,
                title: "Слово дня",
                publication_date: publicationDate.toISOString(),
                expiration_date: publicationDate.add(5, "days").toISOString()
            }
        })
    }

    if (incomingMessage.request?.type == "WidgetGallery") {
        return wrapResponse(incomingMessage,{
            text: "",
            widget_gallery_meta: {
                title: "Заголовок",
                text: "Текст под заголовком",
                image_id: "937455/bb4d5dec66de564a44b9",
                buttons: [
                    {
                        title: 'Галерея картинок',
                        payload: {name: 'picture_gallery'},
                    },
                    {
                        title: '31',
                    }
                ],
            }
        })
    }

    if(incomingMessage.request?.type == "Teasers") {
        return wrapResponse(incomingMessage, {
            text: "",
            teasers_meta: [
                {
                    title: "Заголовок",
                    text: "Текст под заголовком",
                    image_id: "937455/bb4d5dec66de564a44b9",
                },
                {
                    title: "Заголовок 2",
                    text: "Текст под заголовком 2",
                    image_id: "965417/288e96647ca19404e313",
                },
                {
                    title: "Заголовок 3",
                    text: "Текст под заголовком 3",
                    image_id: "965417/a076c957b9f97121f09b",
                },
                {
                    title: "Заголовок 4",
                    text: "Текст под заголовком 4",
                    image_id: "213044/1e42f1b5252df53afeda",
                },
                {
                    title: "Заголовок 5",
                    text: "Текст под заголовком 5",
                    image_id: "937455/bb4d5dec66de564a44b9",
                }
            ]
        })
    }

    if (incomingMessage.request?.type === 'UserAgreements.Accepted') {
        return wrapResponse(incomingMessage, {
            text: 'Ты принял пользовательские соглашения',
        });
    }

    if (incomingMessage.request?.type === 'UserAgreements.Rejected') {
        return wrapResponse(incomingMessage, {
            text: 'Ты НЕ принял пользовательские соглашения',
        });
    }

    if (incomingMessage.account_linking_complete_event) {
        if (headers.authorization?.startsWith('Bearer')) {
            return wrapResponse(incomingMessage, {
                text: 'авторизация прошла успешно',
                tts: 'авторизация прошла успешно',
                end_session: false,
            });
        }

        return wrapResponse(incomingMessage, {
            text: 'Нет авторизационного токена',
            tts: 'Нет авторизационного токена',
            end_session: false,
        });
    }

    if (incomingMessage.request.type?.indexOf('AudioPlayer') === 0) {
        return handlePlayerEvent(incomingMessage);
    }

    const retrieveCurrentSongMeta = (m: IncomingMessage) => {
        const audioState = m.state?.audio_player;
        const token = audioState?.token as keyof typeof songsByToken | undefined;
        const offset = audioState?.offset_ms ?? 0;
        const song = token ? songsByToken[token] : undefined;

        return [song, {offset, token}] as const;
    };

    if (incomingMessage.request.nlu?.intents?.[AudioPlayerIntentNames.Continue]) {
        const [song, {offset, token}] = retrieveCurrentSongMeta(incomingMessage);

        if (!song) {
            return wrapResponse(incomingMessage, {
                text: 'Нечего воспроизводить!',
                tts: 'Нечего воспроизводить!',
                end_session: false,
            });
        }

        return wrapResponse(incomingMessage, {
            text: 'Вот хорошая песня!.',
            directives: {
                audio_player: {
                    action: 'Play',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: offset,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.nlu?.intents?.[AudioPlayerIntentNames.Next]) {
        let [_, {token}] = retrieveCurrentSongMeta(incomingMessage);

        let song = token === 'token' ? songsByToken['token1'] : songsByToken['token'];
        token = token === 'token' ? 'token1' : 'token';

        return wrapResponse(incomingMessage, {
            text: 'Включаю следующий отрывок',
            directives: {
                audio_player: {
                    action: 'Play',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.nlu?.intents?.[AudioPlayerIntentNames.Prev]) {
        let [_, {token}] = retrieveCurrentSongMeta(incomingMessage);

        let song = token === 'token' ? songsByToken['token1'] : songsByToken['token'];
        token = token === 'token' ? 'token1' : 'token';

        return wrapResponse(incomingMessage, {
            text: 'Включаю предыдущий отрывок',
            directives: {
                audio_player: {
                    action: 'Play',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.payload != "" && incomingMessage.request.payload !== undefined && incomingMessage.request.payload !== null) {
        if (incomingMessage.request.payload['name'] == "social_sharing") {
            return wrapResponse(incomingMessage, {
                text: 'Привет от соц шаринга!',
            });
        }
    }

    // payload виджета
    if (incomingMessage.request.payload != "" && incomingMessage.request.payload !== undefined && incomingMessage.request.payload !== null) {
        if (incomingMessage.request.payload['name'] == "picture_gallery") {
            return wrapResponse(incomingMessage, {
                text: 'Посмотри галерею больших картинок с подписями под каждой 61',
                tts: 'Посмотри галер+ею больших картинок с подписями под каждой 61',
                card: {
                    type: 'BigImageList',
                    items: [
                        {
                            image_id: '937455/9b862ab24d8137582bc4',
                            title: 'Картинка 1',
                            description: 'Описание изображения.',
                            button: {
                                text: 'Интересные слова',
                                url: 'http://example.com/',
                                payload: {text: 'Интересные слова'},
                            },
                        },
                        {
                            image_id: '213044/c1b3f1c43889b98da1f5',
                            title: 'Картинка 2',
                            description: 'Описание изображения.',
                            button: {
                                text: 'Словарные слова',
                                payload: {text: 'Словарные слова'},
                            },
                        },
                    ],
                },
                end_session: false,
            });
        }
    }

    if (incomingMessage.request.type === 'ButtonPressed') {
        return wrapResponse(incomingMessage, {
            text: 'type: ButtonPressed ' + JSON.stringify(incomingMessage.request.payload, null, 4),
            tts: 'payload',
        });
    }

    // авторизация
    if (incomingMessage.request.command?.toLowerCase() === 'авторизация') {
        return {
            start_account_linking: {},
            session: message.session,
            version: message.version,
        };
    }

    // авторизация с текстом
    if (incomingMessage.request.command?.toLowerCase() === 'авторизация с текстом') {
        return {
            response: {
                text: 'Книга Чародеи',
                tts: 'Вы покупаете книгу Чародеи'
            },
            start_account_linking: {},
            session: message.session,
            version: message.version,
        };
    }

    if (incomingMessage.request.command?.toLowerCase() === 'логин') {
        return wrapResponse(incomingMessage, {
            text: 'login',
            directives: {
                start_account_linking: {},
            },
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === 'чек логин') {
        if (incomingMessage.session.user.access_token) {
            return wrapResponse(incomingMessage, {
                text: incomingMessage.session.user.access_token?.toString(),
                tts: 'логин выполнен',
                end_session: false,
            });
        }
        return wrapResponse(incomingMessage, {
            text: 'нет директивы access_token',
            end_session: false,
        });
    }


    // Проверка Response
    //основные
    if (incomingMessage.request.command?.toLowerCase() === '0') {
        return wrapResponse(incomingMessage, {
            text: 'добры дзень для тэсту',
            tts: 'добры дзень для тэсту',
            end_session: false,
        });
    }

    // суффикс
    if (incomingMessage.request.command?.toLowerCase() === 'сложный числовой запрос') {
        return wrapResponse(incomingMessage, {
            text: 'сложный числовой запрос',
            end_session: false,
        });
    }

    // отсуствует 'session'
    if (incomingMessage.request.command?.toLowerCase() === '1') {
        return {
            response: {
                text: 'отсутствует session',
                tts: 'отсутствует session',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            version: '1.0',
        };
    }

    // ОШИБКА неверная версия version: '2.0' (true = version: '1.0')
    if (incomingMessage.request.command?.toLowerCase() === '2') {
        return {
            response: {
                text: 'ОШИБКА version 2.0',
                tts: 'ОШИБКА version 2.0',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            session: {
                session_id: '2eac4854-fce721f3-b845abba-20d60',
                message_id: 4,
                user_id: 'AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC',
            },
            version: '2.0',
        };
    }

    // ОШИБКА отсутствует версия 'version'
    if (incomingMessage.request.command?.toLowerCase() === '3') {
        return {
            response: {
                text: 'ОШИБКА отсутствует version',
                tts: 'ОШИБКА отсутствует version',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            session: {
                session_id: '2eac4854-fce721f3-b845abba-20d60',
                message_id: 4,
                user_id: 'AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC',
            },
        };
    }

    // отсуствует 'session_id'
    if (incomingMessage.request.command?.toLowerCase() === '4') {
        return {
            response: {
                text: 'отсутствует session_id',
                tts: 'отсутствует session id',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            session: {
                message_id: 4,
                user_id: 'AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC',
            },
            version: '1.0',
        };
    }
    // session_id
    if (incomingMessage.request.command?.toLowerCase() === '400') {
        return wrapResponse(incomingMessage, {
            text: message.session.session_id,
            tts: 'session id',
            buttons: [
                {
                    title: '400',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // отсуствует 'message_id'
    if (incomingMessage.request.command?.toLowerCase() === '5') {
        return {
            response: {
                text: 'отсутствует message_id',
                tts: 'отсутствует message id',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            session: {
                session_id: '2eac4854-fce721f3-b845abba-20d60',
                user_id: 'AC9WC3DF6FCE052E45A4566A48E6B7193774B84814CE49A922E163B8B29881DC',
            },
            version: '1.0',
        };
    }
    // message_id
    if (incomingMessage.request.command?.toLowerCase() === '500') {
        return wrapResponse(incomingMessage, {
            text: message.session.message_id,
            tts: 'message id',
            buttons: [
                {
                    title: '500',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // отсуствует 'user_id'
    if (incomingMessage.request.command?.toLowerCase() === '6') {
        return {
            response: {
                text: 'отсутствует user_id',
                tts: 'отсутствует user id',
                buttons: [
                    {
                        title: 'Надпись на кнопке',
                        payload: {},
                        url: 'https://example.com/',
                        hide: true,
                    },
                ],
                end_session: false,
            },
            session: {
                session_id: '2eac4854-fce721f3-b845abba-20d60',
                message_id: 4,
            },
            version: '1.0',
        };
    }

    // отсутствует end_session
    if (incomingMessage.request.command?.toLowerCase() === '7') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует end_session',
            tts: 'отсутствует end session',
            buttons: [
                {
                    title: 'нет end_session',
                    url: 'https://www.reddit.com/r/Music/',
                },
            ],
        });
    }

    // ОШИБКА отсутствует 'response.text'
    if (incomingMessage.request.command?.toLowerCase() === '8') {
        return wrapResponse(incomingMessage, {
            tts: 'ОШИБКА отсутствует response.text',
            buttons: [
                {
                    title: 'Нажми на меня',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА отсутствует 'response.text'
    if (incomingMessage.request.command?.toLowerCase() === '9') {
        return wrapResponse(incomingMessage, {
            tts: 'ОШИБКА отсутствует response.text',
            end_session: false,
        });
    }

    // ОШИБКА в поле 'response.text' более 1024 символа
    if (incomingMessage.request.command?.toLowerCase() === '10') {
        return wrapResponse(incomingMessage, {
            text:
                'Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте!Это мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.',
            tts: 'ОШИБКА response.text более 1024',
            buttons: [
                {
                    title: 'Нажми на меня',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // отсутствует 'response.tts'
    if (incomingMessage.request.command?.toLowerCase() === '11') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует response.tts',
            buttons: [
                {
                    title: 'Нажми на меня',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА в поле 'response.tts' более 1024 символа
    if (incomingMessage.request.command?.toLowerCase() === '12') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА response.tts более 1024 символа',
            tts:
                'Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.' +
                'Здравствуйте! мымыЭто мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте!Это мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.Здравствуйте!' +
                'Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.Здравствуйте!Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.' +
                'Здравствуйте! Это мы, хороводоведы.Здравствуйте! Это мы, хороводоведы.',
            buttons: [
                {
                    title: 'Нажми на меня',
                    payload: {},
                    url: 'https://example.com/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // кнопки
    // кнопка button
    if (incomingMessage.request.command?.toLowerCase() === '13') {
        return wrapResponse(incomingMessage, {
            text: 'обычная кнопка',
            tts: 'обычная кнопка',
            buttons: [
                {
                    title: '14',
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // кнопки buttons
    if (incomingMessage.request.command?.toLowerCase() === '14') {
        return wrapResponse(incomingMessage, {
            text: 'модные кнопки',
            tts: 'модные кнопки',
            buttons: [
                {
                    title: 'payload',
                    hide: false,
                    payload: {
                        text: 'привет друг1',
                        url: 'https://www.reddit.com/r/Music/',
                        hide: false,
                    },
                },
                {
                    title: 'empty payload',
                    hide: false,
                    payload: {},
                },
                {
                    title: 'payload 13',
                    payload: {text: '13'},
                },
                {
                    title: '14',
                    hide: false,
                },
                {
                    title: 'payload',
                    hide: true,
                    payload: {
                        text: 'привет друг1',
                        url: 'https://www.reddit.com/r/Music/',
                        hide: true,
                    },
                },
                {
                    title: 'empty payload',
                    hide: true,
                    payload: {},
                },
                {
                    title: 'url',
                    payload: {},
                    url: 'https://www.kitzernograd.ru/%D0%BC%D0%B0%D0%B3%D0%B0%D0%B7%D0%B8%D0%BD/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА отсутствует 'buttons.title'
    if (incomingMessage.request.command?.toLowerCase() === '15') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА отсуствует buttons.title в кнопке',
            tts: 'ОШИБКА отсуствует buttons.title в кнопке',
            buttons: [
                {
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // в 'buttons.title' больше 64 символов
    if (incomingMessage.request.command?.toLowerCase() === '16') {
        return wrapResponse(incomingMessage, {
            text: 'заголовок кнопки больше 64',
            tts: 'заголовок кнопки больше 64',
            buttons: [
                {
                    title: '!!у меня слишком большой заголовок поэтому будет ошибка мой друг!!!!!!!!!!!!у меня',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // отсутствует 'buttons.url'
    if (incomingMessage.request.command?.toLowerCase() === '17') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует url в кнопке',
            tts: 'отсутствует url в кнопке',
            buttons: [
                {
                    title: 'отсутствует url в кнопке',
                    payload: {},
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // отсутствует 'buttons.hide'
    if (incomingMessage.request.command?.toLowerCase() === '18') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует hide в кнопке',
            tts: 'отсутствует hide в кнопке',
            buttons: [
                {
                    title: 'отсутствует hide в кнопке',
                    url: 'https://www.reddit.com/r/Music/',
                    payload: {},
                },
            ],
            end_session: false,
        });
    }

    // текст
    // NER
    if (incomingMessage.request.command?.toLowerCase() === 'пиццу на улицу льва толстого дом 16') {
        return wrapResponse(incomingMessage, {
            text: 'Жди вкусную пиццу ' + incomingMessage.request.nlu.entities,
            tts: 'Жди вкусную пиццу',
            end_session: false,
        });
    }

    // эмоджи
    if (incomingMessage.request.command?.toLowerCase() === '19') {
        return wrapResponse(incomingMessage, {
            text: 'Получи смайлик 🙂🙂🙂',
            tts: 'Получи смайлик',
            end_session: false,
        });
    }

    // эмоджи
    if (incomingMessage.request.command?.toLowerCase() === '🙂') {
        return wrapResponse(incomingMessage, {
            text: '🙂🙂🙂🙂',
            tts: 'Получи еще смайлик',
            end_session: false,
        });
    }

    // интерпретация символов М
    if (incomingMessage.request.command?.toLowerCase() === '20') {
        return wrapResponse(incomingMessage, {
            text: '&#77;',
            tts: '&#77;',
            end_session: false,
        });
    }

    // перенос 1
    if (incomingMessage.request.command?.toLowerCase() === '21') {
        return wrapResponse(incomingMessage, {
            text: 'В МИДе прокомментировали учения НАТО Trident Junction. \\\n Интер\nесно?',
            tts: 'В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?"',
            buttons: [
                {
                    title: 'перенос 1',
                    url: 'https://iz.ru/804772/2018-10-25/v-mid-prokommentirovali-ucheniia-nato-trident-junction',
                },
            ],
            end_session: false,
        });
    }

    // перенос 2
    if (incomingMessage.request.command?.toLowerCase() === '22') {
        return wrapResponse(incomingMessage, {
            text:
                'Мне кажется, что буква Ш больше похожа на вилку. Такую ложку я никогда не видел! И в приниципе это очень длинная строка которая не вмещается на экран.\n' +
                '\n' +
                'Упр 1:\n' +
                'Открой рот.\n' +
                '\n' +
                'Упра 2:\n' +
                'Сожми губы и бла бла.\n' +
                '\n' +
                'Упраж 3:\n' +
                'Расслабь булки...\n' +
                '\n' +
                'Упражне 5:\n' +
                'Крикни ртом...\n' +
                '\n' +
                '6\n' +
                '7\n' +
                '8\n' +
                '9\n' +
                '10\n' +
                '\n' +
                'Повторим или ты хочешь пойти дальше?',
            tts: 'тут длинный текст с переносами, смотри чтобы шторка на станции отрисовала корректно',
            buttons: [
                {
                    title: 'перенос 2',
                    url: 'https://iz.ru/804772/2018-10-25/v-mid-prokommentirovali-ucheniia-nato-trident-junction',
                },
            ],
            end_session: false,
        });
    }

    // перенос 3
    if (incomingMessage.request.command?.toLowerCase() === '23') {
        return wrapResponse(incomingMessage, {
            text: 'В МИДе прокомментировали учения НАТО Trident Junction. \n Интер\nесно?',
            tts: 'В МИДе прокомментировали учения НАТО Trident Junction. - Интересно?"',
            buttons: [
                {
                    title: 'перенос 3',
                    url: 'https://iz.ru/804772/2018-10-25/v-mid-prokommentirovali-ucheniia-nato-trident-junction',
                },
            ],
            end_session: false,
        });
    }

    // саджесты
    if (incomingMessage.request.command?.toLowerCase() === '24') {
        return wrapResponse(incomingMessage, {
            text: 'лови саджесты',
            tts: 'лови саджесты',
            // card: {
            //     type: 'BigImage',
            //     image_id: '1030494/c001acff852fff60280c',
            //     title: 'лови картинку card.title',
            //     description: 'лови картинку card.description',
            //     button: {
            //         text: 'лови картинку card.button.text',
            //         url: 'https://www.reddit.com/r/Music/',
            //     },
            // },
            buttons: [
                {
                    title: '1 url',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
                {
                    title: '4',
                    hide: true,
                },
                {
                    title: 'Да',
                    hide: true,
                },
                {
                    title: '4 payload clear',
                    payload: {},
                    hide: true,
                },
                {
                    title: '24',
                    hide: true,
                },
                {
                    title: 'как дела + url',
                    payload: {text: 'как дела'},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
                {
                    title: 'кнопка hide=false',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // регистрозависимый саджест для команды 24
    if (incomingMessage.request.command === 'Да') {
        return wrapResponse(incomingMessage, {
            text: 'вы написали "Да" с большой буквы',
            tts: 'вы написали "Да" с большой буквы',
            end_session: false,
        });
    }

    // саджесты
    if (incomingMessage.request.command?.toLowerCase() === '241') {
        return wrapResponse(incomingMessage, {
            text: 'саджесты Big Image',
            tts: 'саджесты Big Image',
            card: {
                type: 'BigImage',
                image_id: '1652229/b04816d65093aade29e3',
                title: 'картинка',
                button: {
                    text: 'картинка',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Реддит про политику',
                    url: 'https://www.reddit.com/r/Politics/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: false,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // саджесты
    if (incomingMessage.request.command?.toLowerCase() === '243') {
        return wrapResponse(incomingMessage, {
            text: 'саджесты Big Image',
            tts: 'саджесты Big Image',
            card: {
                type: 'BigImage',
                image_id: '1652229/b04816d65093aade29e3',
                title: 'картинка',
                button: {
                    text: 'картинка',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Реддит про политику',
                    url: 'https://www.reddit.com/r/Politics/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: false,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: true,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // саджест с message_id
    if (incomingMessage.request.command?.toLowerCase() === '242') {
        return wrapResponse(incomingMessage, {
            text: 'message_id - ' + message.session.message_id + '\nsession_id - ' + message.session.session_id,
            tts: 'саджест с message id и session id',
            buttons: [
                {
                    title: '242',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // картинки
    // картинка 1 пустая
    if (incomingMessage.request.command?.toLowerCase() === '25') {
        return wrapResponse(incomingMessage, {
            text: 'пустая картинка',
            tts: 'пустая картинка',
            card: {
                type: 'BigImage',
                image_id: '1652229/d7ca681fdd1d1acab9b1',
                title: 'картинка',
                button: {
                    text: 'картинка',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Надпись на кнопке',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }
    // картинка 250
    if (incomingMessage.request.command?.toLowerCase() === '250') {
        return wrapResponse(incomingMessage, {
            text: 'картинка 250',
            tts: 'картинка 250',
            card: {
                type: 'BigImage',
                image_id: '1652229/b04816d65093aade29e3',
                title: 'title картинки',
                button: {
                    text: 'кнопка картинки',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: '250',
                    hide: true,
                },
                {
                    title: '250',
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // картинка 2
    if (incomingMessage.request.command?.toLowerCase() === '26') {
        return wrapResponse(incomingMessage, {
            text: 'лови картинку',
            tts: 'лови картинку',
            card: {
                type: 'BigImage',
                image_id: '1652229/b04816d65093aade29e3',
                title: 'лови картинку 1',
                description: 'лови картинку 2 \nТекст с новой строки <br>Новая строка',
                button: {
                    text: 'лови картинку 3',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'лови картинку',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА отсутствует 'card.image_id'
    if (incomingMessage.request.command?.toLowerCase() === '27') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА отсутствует card.image_id',
            tts: 'ОШИБКА отсутствует image id',
            card: {
                type: 'BigImage',
                title: 'Заголовок для изображения',
                description: 'Описание изображения.',
                button: {
                    text: 'Надпись на кнопке',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Надпись на кнопке',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА большой 'card.image_id'
    if (incomingMessage.request.command?.toLowerCase() === '28') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА большой card.image_id',
            tts: 'ОШИБКА большой card.image_id',
            card: {
                type: 'BigImage',
                image_id: '3F10D5857AB85CFE4C31635E46DAE9D40ED4505DBB67C3E1AC95DB2614241D70',
                title: 'лови картинку',
                description: 'лови картинку',
                button: {
                    text: 'лови картинку',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'лови картинку',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // отсутствует 'card.type'
    if (incomingMessage.request.command?.toLowerCase() === '29') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует card.type',
            tts: 'отсутствует card type',
            card: {
                image_id: '1652229/c199f41cab241ae42d9c',
                title: 'картинка',
                description: 'картинка',
                button: {
                    text: 'картинка',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Надпись на кнопке',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // отсутствует 'card.title'
    if (incomingMessage.request.command?.toLowerCase() === '30') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует card.title',
            tts: 'отсутствует card title',
            card: {
                type: 'BigImage',
                image_id: '1540737/05ad8795138acd5a169a',
                description: 'картинка',
                button: {
                    text: 'картинка',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'Надпись на кнопке',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // галлерея картинок
    if (incomingMessage.request.command?.toLowerCase() === '31') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри на галерею картинок',
            tts: 'Посмотри на галер+ею картинок',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галерея картинок',
                },
                items: [
                    {
                        image_id: '1652229/290b313ea588a8771bba',
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                        button: {
                            text: 'make pepe great again.',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'Кнопка "ТЕСТ"',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // галлерея картинок
    if (incomingMessage.request.command?.toLowerCase() === '311') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри на галерею картинок',
            tts: 'Посмотри на галер+ею картинок',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галерея картинок',
                },
                items: [
                    {
                        image_id: '1652229/290b313ea588a8771bba',
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                        button: {
                            text: 'make pepe great again.',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'Реддит про политику',
                    url: 'https://www.reddit.com/r/Politics/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: false,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: true,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '310') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри галерею картинок 310',
            tts: 'Посмотри галер+ею картинок 310',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галерея картинок',
                },
                items: [
                    {
                        image_id: '213044/d746c31f581e9b0e372f',
                        title: 'Интересные слова',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        title: 'Словарные слова',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                ],
            },
            buttons: [
                {
                    title: 'Кнопка "ТЕСТ 1"',
                    payload: {text: 'ТЕСТ 1'},
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 2"',
                    payload: {text: 'ТЕСТ 2'},
                    url: 'http://example.com/',
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 3"',
                    payload: {text: 'ТЕСТ 3'},
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // текстовые карточки
    // множественные карточки (Громов PASKILLS-6709)
    // вместе с дополнительными кнопками (Мокеев PASKILLS-4894)
    if (incomingMessage.request.command?.toLowerCase() === '312') {
        return wrapResponse(incomingMessage, {
            "text": "Геолокация",
            "tts": "Геолокация",
            "end_session": false,
            "directives": {
                "request_geolocation": {}
            },
            "card": {
                "type": "BigImage",
                "image_id": "1652229/16b371e1747fa842018d",
                "button": {
                    "text": "Надпись на кнопке"
                }
            },
            "buttons": [
                {
                    "title": "Надпись на кнопке",
                    "payload": {},
                    "hide": true
                },
                {
                    "title": "Надпись на кнопке",
                    "payload": {},
                    "hide": false
                }
            ],
        });
    }

    // галерея картинок отсутствует 'items.type'
    if (incomingMessage.request.command?.toLowerCase() === '32') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствует items.type',
            tts: 'отсутствует items type',
            card: {
                header: {
                    text: 'Галлерея картинок',
                },
                items: [
                    {
                        image_id: '1652229/290b313ea588a8771bba',
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                    },

                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'нопка "тест',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // галерея картинок отсутствуют 'items.image_id'
    if (incomingMessage.request.command?.toLowerCase() === '33') {
        return wrapResponse(incomingMessage, {
            text: 'отсутствуют items.image_id',
            tts: 'отсутствуют items image id',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галлерея картинок',
                },
                items: [
                    {
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                        button: {
                            text: 'make pepe great again.',
                            payload: {},
                        },
                    },
                    {
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },

                    {
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'нопка "тест',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА в 'items' > 5 карточек
    if (incomingMessage.request.command?.toLowerCase() === '34') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА в items > 5 карточек',
            tts: 'ОШИБКА в items больше 5 карточек',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галлерея картинок',
                },
                items: [
                    {
                        image_id: '1652229/290b313ea588a8771bba',
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                        button: {
                            text: 'make pepe great again.',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },

                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'нопка "тест',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ОШИБКА неправильный 'card.items'
    if (incomingMessage.request.command?.toLowerCase() === '35') {
        return wrapResponse(incomingMessage, {
            text: 'ОШИБКА неправильный card.items',
            tts: 'ОШИБКА неправильный card items',
            card: {
                type: 'ItemsList',
                header: {
                    text: 'Галлерея картинок',
                },
                xxxx: [
                    {
                        image_id: '1652229/290b313ea588a8771bba',
                        title: 'make pepe great again.',
                        description: 'make pepe great again.',
                        button: {
                            text: 'make pepe great again.',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },

                    {
                        image_id: '965417/9d7cbcd04feae6ef7dbd',
                        title: 'make pepe again',
                        description: 'make pepe again',
                        button: {
                            text: 'make pepe again',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                    {
                        image_id: '1521359/6bc78d704b527fc22f23',
                        title: 'steins gate',
                        description: 'steins gate',
                        button: {
                            text: 'steins gate',
                            url: 'http://example.com/',
                            payload: {},
                        },
                    },
                ],
                footer: {
                    text: 'Кнопка "тест"',
                    button: {
                        text: 'Нажми на кнопку',
                        url: 'https://example.com/',
                        payload: {},
                    },
                },
            },
            buttons: [
                {
                    title: 'нопка "тест',
                    payload: {},
                    url: 'https://example.com/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // звуки и эффекты
    // звук 1 эффект голоса
    if (incomingMessage.request.command?.toLowerCase() === '36') {
        return wrapResponse(incomingMessage, {
            text: 'звук 1 и картинка',
            tts: '<speaker effect="megaphone">лови картинку и звук<speaker effect="-">мой друг',
            card: {
                type: 'BigImage',
                image_id: '1030494/f0fd403689f175b03116',
                title: 'картинка 1',
                description: 'картинка 2',
                button: {
                    text: 'картинка 3',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'картинка 4',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // звук 2 длинный
    if (incomingMessage.request.command?.toLowerCase() === '37') {
        return wrapResponse(incomingMessage, {
            text: 'звук 2',
            tts:
                '<speaker audio="dialogs-upload/026ac948-d518-4fd4-a103-8b9d8cce6cd8/8c55bdaf-cefe-4a5c-8363-16013cd8c455.opus">Привет',
            end_session: false,
        });
    }

    // звук 3 - 8 бит
    if (incomingMessage.request.command?.toLowerCase() === '38') {
        return wrapResponse(incomingMessage, {
            text: 'звук 3 - 8 бит',
            tts:
                '<speaker audio="dialogs-upload/7e160623-23e0-45bd-98df-15741100b508/c45d817c-186d-40f4-bda9-2c585109d817.opus">Слушай чиптюн',
            end_session: false,
        });
    }

    // звук 4 + картинка
    if (incomingMessage.request.command?.toLowerCase() === '39') {
        return wrapResponse(incomingMessage, {
            text: 'звук 4 и картинка',
            tts: '<speaker audio="alice-music-drums-3.opus">',
            card: {
                type: 'BigImage',
                image_id: '1030494/f0fd403689f175b03116',
                title: 'картинка 1',
                description: 'картинка 2',
                button: {
                    text: 'картинка 3',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'картинка 4',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // звук 5
    if (incomingMessage.request.command?.toLowerCase() === '40') {
        return wrapResponse(incomingMessage, {
            text: 'звук 5',
            tts: '<speaker audio="alice-sounds-game-win-1.opus"> У вас что-то получилось!',
            buttons: [
                {
                    title: 'Кнопка. У вас что-то получилось!',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    // ошибки в JSON
    // ошибки в JSON 1
    if (incomingMessage.request.command?.toLowerCase() === '41') {
        return wrapResponse(incomingMessage, {
            abc: '123hgj',
            ttts: 'ffwef',
            abdsabsdjk: 'asdjgj: {{{{{{[[][][][][',
            end_session: false,
        });
    }

    // ошибки в JSON 2
    if (incomingMessage.request.command?.toLowerCase() === '42') {
        return wrapResponse(incomingMessage, {
            abc: '123hgjRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR',
            ttts: 'ffwefRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR',
            abdsabsdjk: 'asdjgj: {{{{{{[[][][][][4234234234234',
            end_session: false,
        });
    }

    // ошибки в JSON 3
    if (incomingMessage.request.command?.toLowerCase() === '43') {
        return wrapResponse(incomingMessage, {
            ацауац: '&#77;',
            dvwef: 'лови картинку',
            cwfewef: {
                ewfwef: 'BigImage',
                image_id: '&#2',
                title: 'лови картинку',
                description: 'лови картинку',
                button: {
                    text: 'лови картинку',
                    url: 'https://www.reddit.com/r/Music/',
                },
            },
            buttons: [
                {
                    title: 'лови картинку',
                    payload: {},
                    url: 'https://www.reddit.com/r/Music/',
                    hide: true,
                },
            ],
            efweffewn: false,
            end_session: false,
        });
    }

    // end_session
    // end_session = true
    if (incomingMessage.request.command?.toLowerCase() === '44') {
        return wrapResponse(incomingMessage, {
            text: 'end_session = true',
            tts: 'end session = true',
            end_session: true,
        });
    }

    // end_session = false
    if (incomingMessage.request.command?.toLowerCase() === '45') {
        return wrapResponse(incomingMessage, {
            text: 'end_session = false',
            tts: 'end session = false',
            end_session: false,
        });
    }

    // change session state
    if (incomingMessage.request.command?.toLowerCase() === '46') {
        const state = incomingMessage.state || {};
        const val = (state.session || {}).val || 0;

        return wrapResponse(
            incomingMessage,
            {
                text: 'состояние сессии - ' + val,
                tts: 'состояние сессии - ' + val,
                end_session: false,
            },
            {val: val + 1},
        );
    }

    // change user state
    if (incomingMessage.request.command?.toLowerCase() === '47') {
        const state = incomingMessage.state || {};
        const val = (state.user || {}).val || 0;

        return wrapResponse(
            incomingMessage,
            {
                text: 'состояние пользователя - ' + val,
                tts: 'состояние пользователя - ' + val,
                end_session: false,
            },
            state.session,
            {val: val + 1},
        );
    }

    // change session and user state
    if (incomingMessage.request.command?.toLowerCase() === '48') {
        const state = incomingMessage.state || {};
        const sessionVal = (state.session || {}).val || 0;
        const userVal = (state.user || {}).val || 0;

        return wrapResponse(
            incomingMessage,
            {
                text: 'состояние сессии - ' + sessionVal + ' а пользователя - ' + userVal,
                tts: 'состояние сессии - ' + sessionVal + ' а пользователя - ' + userVal,
                end_session: false,
            },
            {val: sessionVal + 1},
            {val: userVal + 1},
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '49') {
        const state = incomingMessage.state || {};
        const user_state = state.user || {};
        const keys = Object.keys(user_state);
        const user_state_update: State = {};
        for (const key of keys) {
            user_state_update[key] = null;
        }

        return wrapResponse(
            incomingMessage,
            {
                text: 'Удаляю состояние пользователя',
                tts: 'Удаляю состояние пользователя',
                end_session: false,
            },
            state.session,
            user_state_update,
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '50') {
        const token = 'token';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            text: 'Вот хорошая песня',
            tts: 'Вот хорошая песня',
            directives: {
                audio_player: {
                    action: 'Play',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: false,
        });
    }
    if (incomingMessage.request.command?.toLowerCase() === '51') {
        const token = 'token2';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            text: 'Вот хорошая песня c отступом в 33 секунды',
            tts: 'Вот хорошая песня c отступом в 33 секунды',
            directives: {
                audio_player: {
                    action: 'Play',
                    background_mode: 'pause',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 33000,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: true,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '52') {
        return wrapResponse(incomingMessage, {
            text: 'Останавливаю плеер',
            tts: 'Останавливаю плеер',
            directives: {
                audio_player: {
                    action: 'Stop',
                },
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '53') {
        const token = 'token1';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            text: 'Песня с паузой и выходом из навыка',
            tts: 'Песня с паузой и выходом из навыка',
            directives: {
                audio_player: {
                    action: 'Play',
                    background_mode: 'pause',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: true,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '54') {
        const token = 'token1';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            text: 'Песня с приглушением и выходом из навыка',
            tts: 'Песня с приглушением и выходом из навыка',
            directives: {
                audio_player: {
                    action: 'Play',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: true,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '55') {
        const token = 'token2';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            text: 'Вот хорошая длинная песня!.',
            tts: 'Вот хорошая длинная песня!.',
            directives: {
                audio_player: {
                    action: 'Play',
                    background_mode: 'pause',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: false,
        });
    }

    // play without text with end session
    if (incomingMessage.request.command?.toLowerCase() === '56') {
        const token = 'token3';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            directives: {
                audio_player: {
                    action: 'Play',
                    background_mode: 'pause',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: true,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '60') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри галерею больших картинок 60',
            tts: 'Посмотри галер+ею больших картинок 60',
            card: {
                type: 'BigImageList',
                header: {
                    text: 'Галерея картинок',
                },
                items: [
                    {
                        image_id: '1540737/2584e279c93fdf6824fc',
                        title: 'Картинка 1',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/8a6f9390f02971a10dc8',
                        title: 'Картинка 2',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '1521359/e729183f87093bd3a81c',
                        title: 'Картинка 3',
                    },
                    {
                        image_id: '937455/bb4d5dec66de564a44b9',
                        title: 'Картинка 4',
                    },
                    {
                        image_id: '965417/288e96647ca19404e313',
                        title: 'Картинка 5',
                    },
                    {
                        image_id: '965417/a076c957b9f97121f09b',
                        title: 'Картинка 6',
                    },
                    {
                        image_id: '213044/1e42f1b5252df53afeda',
                        title: 'Картинка 7',
                    },
                ],
            },
            buttons: [
                {
                    title: 'Кнопка "ТЕСТ 1"',
                    payload: {text: 'ТЕСТ 1'},
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 2"',
                    payload: {text: 'ТЕСТ 2'},
                    url: 'http://example.com/',
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 3"',
                    payload: {text: 'ТЕСТ 3'},
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '61') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри галерею больших картинок с подписями под каждой 61',
            tts: 'Посмотри галер+ею больших картинок с подписями под каждой 61',
            card: {
                type: 'BigImageList',
                items: [
                    {
                        image_id: '937455/9b862ab24d8137582bc4',
                        title: 'Картинка 1',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        title: 'Картинка 2',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                ],
            },
            end_session: false,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '62') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри галерею больших картинок с подписями под каждой 62',
            tts: 'Посмотри галер+ею больших картинок с подписями под каждой 62',
            card: {
                type: 'ImageGallery',
                header: {
                    text: 'Специально очень длинный длинный длинный длинный длинный хедер'
                },
                items: [
                    {
                        image_id: '937455/9b862ab24d8137582bc4',
                        title: 'Картинка 1',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        title: 'Картинка 2',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '937455/9b862ab24d8137582bc4',
                        title: 'Картинка 1 только название',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        description: 'Только описание изображения.',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                ],
            },
            end_session: false,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '63') {
        return wrapResponse(incomingMessage, {
            text: 'Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0.',
            tts: 'Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0. Очень длинный заголовок 0.',
            card: {
                type: 'BigImageList',
                header: {
                    text: 'Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1. Очень длинный заголовок 1.',
                },
                items: [
                    {
                        image_id: '1540737/2584e279c93fdf6824fc',
                        title: 'Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2. Очень длинный заголовок 2.',
                        description: 'Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9.',
                        button: {
                            text: 'Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3. Очень длинный заголовок 3.',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/8a6f9390f02971a10dc8',
                        title: 'Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4. Очень длинный заголовок 4.',
                        description: 'Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9.',
                        button: {
                            text: 'Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5. Очень длинный заголовок 5.',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '1521359/e729183f87093bd3a81c',
                        title: 'Картинка 3',
                    },
                    {
                        image_id: '937455/bb4d5dec66de564a44b9',
                        title: 'Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6. Очень длинный заголовок 6.',
                    },
                    {
                        image_id: '965417/288e96647ca19404e313',
                        title: 'Картинка 5',
                    },
                    {
                        image_id: '965417/a076c957b9f97121f09b',
                        description: 'Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9. Очень длинный заголовок 9.',
                        title: 'Картинка 6',
                    },
                    {
                        image_id: '213044/1e42f1b5252df53afeda',
                        title: 'Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8. Очень длинный заголовок 8.',
                    },
                ],
            },
            buttons: [
                {
                    title: 'Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7. Очень длинный заголовок 7.',
                    payload: {text: 'ТЕСТ 1'},
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 2"',
                    payload: {text: 'ТЕСТ 2'},
                    url: 'http://example.com/',
                    hide: true,
                },
                {
                    title: 'Кнопка "ТЕСТ 3"',
                    payload: {text: 'ТЕСТ 3'},
                    hide: false,
                },
            ],
            end_session: false,
        });
    }

    // payload в Card
    if (incomingMessage.request.command?.toLowerCase() === '64') {
        return wrapResponse(incomingMessage, {
            text: 'Посмотри галерею больших картинок с подписями под каждой 62',
            tts: 'Посмотри галер+ею больших картинок с подписями под каждой 62',
            card: {
                type: 'ImageGallery',
                header: {
                    text: 'Специально очень длинный длинный длинный длинный длинный хедер'
                },
                items: [
                    {
                        image_id: '937455/9b862ab24d8137582bc4',
                        title: 'Картинка 1',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        title: 'Картинка 2',
                        description: 'Описание изображения.',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '937455/9b862ab24d8137582bc4',
                        title: 'Картинка 1 только название',
                        button: {
                            text: 'Интересные слова',
                            url: 'http://example.com/',
                            payload: {text: 'Интересные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        description: 'Только описание изображения.',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                    {
                        image_id: '213044/c1b3f1c43889b98da1f5',
                        button: {
                            text: 'Словарные слова',
                            payload: {text: 'Словарные слова'},
                        },
                    },
                ],
            },
            buttons: [
                {
                    title: 'Реддит про политику',
                    url: 'https://www.reddit.com/r/Politics/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: false,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: false,
                },
                {
                    title: 'News',
                    url: 'https://www.reddit.com/r/News/',
                    hide: true,
                },
                {
                    title: 'Очень длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное длинное название кнопки',
                    url: 'https://www.reddit.com/r/Technology/',
                    hide: true,
                },
            ],
            end_session: false,
        });
    }

    const slotValuesToList = (slots: Slots) => {
        const slotNames = Object.keys(slots);
        const arr = [];

        for (const name of slotNames) {
            arr.push(slots[name].value);
        }

        return arr;
    };

    const slotsToStr = (slots: Slots) => {
        let slotsStr = '';
        const names = Object.keys(slots);
        for (let i = 0; i < names.length; i++) {
            slotsStr += names[i] + ': ' + slots[names[i]].value
            if (i < names.length - 1)
                slotsStr += ', ';
        }

        return slotsStr;
    };

    // TopUpPhone intent (implicit discovery)
    const intents = incomingMessage.request.nlu?.intents
    if (intents != null && intents[ImplicitDiscoveryIntentNames.TopUpPhone]) {
        const answer = 'Вижу интент, подходящий для implicit discovery: ' + ImplicitDiscoveryIntentNames.TopUpPhone
            + '. Слоты: ' + slotsToStr(intents[ImplicitDiscoveryIntentNames.TopUpPhone].slots)

        return wrapResponse(incomingMessage, {
            text: answer,
            tts: answer,
            end_session: false,
        });
    }

    // intents
    if (incomingMessage.request.command?.toLowerCase() === 'эй включи свет в ванной') {
        const slotList = slotValuesToList(incomingMessage.request.nlu.intents!.test_intent.slots);

        return wrapResponse(incomingMessage, {
            text: 'Интенты: ' + slotList.toString(),
            tts: 'Интенты: ' + slotList.toString(),
            end_session: false,
        });
    }
    // intents yandex_type
    if (incomingMessage.request.command?.toLowerCase() === 'число 13') {
        const slotList = slotValuesToList(incomingMessage.request.nlu.intents!.yandex_type.slots);

        return wrapResponse(incomingMessage, {
            text: 'Интенты: ' + slotList.toString(),
            tts: 'Интенты: ' + slotList.toString(),
            end_session: false,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === 'число 14') {
        const slotList = slotValuesToList(incomingMessage.request.nlu.intents!.yandex_type.slots);

        return wrapResponse(incomingMessage, {
            text: 'Интенты: ' + slotList.toString(),
            tts: 'Интенты: ' + slotList.toString(),
            end_session: true,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === 'число 15') {
        const token = 'token3';
        const song = songsByToken[token];

        return wrapResponse(incomingMessage, {
            directives: {
                audio_player: {
                    action: 'Play',
                    background_mode: 'pause',
                    item: {
                        stream: {
                            url: song.url,
                            offset_ms: 0,
                            token,
                        },
                        metadata: song.metadata,
                    },
                },
            },
            end_session: true,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === 'число 16') {
        const slotList = slotValuesToList(incomingMessage.request.nlu.intents!.yandex_type.slots);

        return wrapResponse(incomingMessage, {
            text: 'Интенты: ' + slotList.toString(),
            tts: 'Интенты: ' + slotList.toString(),
            directives: {
                audio_player: {
                    action: 'Stop',
                },
            },
            end_session: false,
        });
    }

    // задержки
    function sleep(ms: number) {
        return new Promise((resolve) => setTimeout(resolve, ms));
    }

    // задержка 2 сек
    if (incomingMessage.request.command?.toLowerCase() === '2 секунды') {
        await sleep(2000);
        return wrapResponse(incomingMessage, {
            text: '2 секунды',
            tts: '2 секунды',
            end_session: false,
        });
    }

    // задержка 2.9 сек
    if (incomingMessage.request.command?.toLowerCase() === '3 секунды') {
        await sleep(2900);
        return wrapResponse(incomingMessage, {
            text: '3 секунды',
            tts: '3 секунды',
            end_session: false,
        });
    }

    // задержка 4 сек
    if (incomingMessage.request.command?.toLowerCase() === '4 секунды') {
        await sleep(4000);
        return wrapResponse(incomingMessage, {
            text: '4 секунды',
            tts: '4 секунды',
            end_session: false,
        });
    }

    // мат
    if (incomingMessage.request.command?.toLowerCase() === 'мат') {
        return wrapResponse(incomingMessage, {
            text: 'блять',
            tts: 'блять',
            buttons: [
                {
                    title: 'блять',
                    hide: false
                },
                {
                    title: 'блять',
                    hide: true
                },
            ],
            end_session: false,
        });
    }

    // точка
    if (incomingMessage.request.command?.toLowerCase() === 'точка') {
        return wrapResponse(incomingMessage, {
            text: '.',
            tts: '.',
            end_session: false,
        });
    }

    // пустое поле text
    if (incomingMessage.request.command?.toLowerCase() === 'включи музыку') {
        return wrapResponse(incomingMessage, {
            text: '',
            end_session: false,
        });
    }

    // for test
    if (incomingMessage.request.command?.toLowerCase() === 'тест') {
        return wrapResponse(incomingMessage, {
            text: 'тест',
            tts: 'тест',
            buttons: [
                {
                    title: 'посмотреть код',
                    payload: null,
                    url: 'https://github.com/alexvolchetsky/yandex.alice.sdk',
                    hide: false
                },
                {
                    title: 'ответ без изображений',
                    payload: null,
                    url: null,
                    hide: false
                }
            ],
            end_session: false,
        });
    }

    // purchase in skill
    if (incomingMessage.request.command?.toLowerCase() === 'закажи пиццу') {
        return wrapResponse(incomingMessage, {
            text: 'Ваш заказ',
            tts: 'Ваш заказ',
            directives: {
                start_purchase: {
                    purchase_request_id: uuidv4(),
                    image_url: 'http://url_to_image_purchase',
                    caption: 'caption',
                    description: 'description',
                    currency: 'RUB',
                    type: 'BUY',
                    payload: {},
                    merchant_key: 'affee793-0166-48ff-8bc9-adb62ec5ca8c',
                    products: [
                        {
                            product_id: '5e4cf57a-8497-11ea-bc55-0242ac130209',
                            title: 'title 1',
                            user_price: '1',
                            price: '110',
                            quantity: '1',
                            nds_type: 'nds_18'
                        }
                    ],
                    delivery: {
                        city: 'Москва',
                        street: 'Паромная',
                        index: '27001',
                        house: 'д. 5',
                        building: 'к. 2',
                        floor: 'этаж 4',
                        flat: 'кв. 13',
                        porch: 'п. 45',
                        price: '1',
                        nds_type: 'nds_18'
                    }
                },
            },
            end_session: false,
        });
    }

    //  test purchase in skill
    if (incomingMessage.request.command?.toLowerCase() === 'закажи пиццу бесплатно') {
        return wrapResponse(incomingMessage, {
            text: 'Ваш бесплатный заказ',
            tts: 'Ваш бесплатный заказ',
            directives: {
                start_purchase: {
                    purchase_request_id: uuidv4(),
                    image_url: 'http://url_to_image_purchase',
                    caption: 'caption',
                    description: 'description',
                    currency: 'RUB',
                    type: 'BUY',
                    payload: {},
                    merchant_key: 'affee793-0166-48ff-8bc9-adb62ec5ca8c',
                    products: [
                        {
                            product_id: '5e4cf57a-8497-11ea-bc55-0242ac130209',
                            title: 'title 1',
                            user_price: '1',
                            price: '110',
                            quantity: '1',
                            nds_type: 'nds_18'
                        }
                    ],
                    test_payment: true,
                    delivery: {
                        city: 'Москва',
                        street: 'Паромная',
                        index: '27001',
                        house: 'д. 5',
                        building: 'к. 2',
                        floor: 'этаж 4',
                        flat: 'кв. 13',
                        porch: 'п. 45',
                        price: '1',
                        nds_type: 'nds_18'
                    }
                },
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.type === 'Purchase.Confirmation') {
        return wrapResponse(incomingMessage, {
                text: "Заказ готов, ждите пиццу",
                tts: "Заказ готов, ждите пиццу",
                end_session: false,
                directives: {
                    confirm_purchase: {}
                }
            }
        );
    }

    if (incomingMessage.request.type === 'Purchase.Complete') {
        return wrapResponse(incomingMessage, {
                text: "Заказ готов, ждите бургер",
                tts: "Заказ готов, ждите бургер",
                end_session: false
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === 'квест') {
        return wrapResponse(
            incomingMessage,
            {
                text: 'Нажми лапку у игрушки, чтобы я послушала музыку',
                tts: 'Нажми лапку у игрушки, чтобы я послушала музыку',
                directives: {
                    activate_skill_product: {
                        activation_type: 'music'
                    }
                },
                end_session: false,
            },
            {
                is_quest: true
            }
        );
    }

    if (incomingMessage.request.type === 'SkillProduct.Activated' && incomingMessage.state?.session?.is_quest) {
        return wrapResponse(incomingMessage, {
                text: "Привет, юный искатель квестов! Мы отправляемся в незабываемое путешествие",
                tts: "Привет, юный искатель квестов! Мы отправляемся в незабываемое путешествие",
                end_session: false
            }
        );
    }


    if (incomingMessage.request.command?.toLowerCase() === 'активируй смешариков'
        || incomingMessage.request.command?.toLowerCase() === 'активируй игрушку'
        || incomingMessage.request.command?.toLowerCase() === 'активируй холодное сердце'
    ) {
        return wrapResponse(
            incomingMessage,
            {
                text: 'Нажми лапку у игрушки, чтобы я послушала музыку',
                tts: 'Нажми лапку у игрушки, чтобы я послушала музыку',
                directives: {
                    activate_skill_product: {
                        activation_type: 'music'
                    }
                },
                end_session: false,
            },
            {
                is_music_toy_activation: true
            }
        );
    }

    if (incomingMessage.request.type === 'SkillProduct.Activated'
        && incomingMessage.state?.session?.is_music_toy_activation
    ) {
        return wrapResponse(incomingMessage, {
                text: 'Активирована игрушка: ' + incomingMessage.request.product_name,
                tts: 'Активирована игрушка ' + incomingMessage.request.product_name,
                end_session: false
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === 'какой продукт активируется') {
        return wrapResponse(
            incomingMessage,
            {
                text: 'Запусти музыку',
                tts: 'Запусти музыку',
                directives: {
                    activate_skill_product: {
                        activation_type: 'music'
                    }
                },
                end_session: false,
            },
            {
                product_activation_check: true
            }
        );
    }

    if (incomingMessage.request.type === 'SkillProduct.Activated'
        && incomingMessage.state?.session?.product_activation_check
    ) {
        return wrapResponse(incomingMessage, {
                text: 'Музыка активирует продукт: ' + incomingMessage.request.product_name,
                tts: 'Музыка активирует продукт ' + incomingMessage.request.product_name,
                end_session: false
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === 'какие продукты активированы'
        || incomingMessage.request.command?.toLowerCase() === 'какие игрушки активированы'
    ) {
        var allProducts = incomingMessage.session.user.skill_products
            ?.map(product => product.name)
            .join(", ");
        if (allProducts === null || allProducts === undefined || allProducts === '') {
            allProducts = "У вас ничего не активированно"
        } else {
            allProducts = "Активированны: " + allProducts;
        }
        return wrapResponse(incomingMessage, {
            text: allProducts,
            tts: allProducts,
            end_session: false
        });
    }

    if (incomingMessage.request.type === 'SkillProduct.ActivationFailed') {
        if (incomingMessage.request?.error === 'music_not_playing') {
            return wrapResponse(incomingMessage, {
                    text: 'Я не слышу мелодию',
                    tts: 'Я не слышу мелодию',
                    end_session: false
                }
            );
        } else if (incomingMessage.request?.error === 'music_not_recognized') {
            return wrapResponse(incomingMessage, {
                    text: 'Эту мелодию я не знаю',
                    tts: 'Эту мелодию я не знаю',
                    end_session: false
                }
            );
        }
    }

    if (incomingMessage.request.command?.toLowerCase() === 'гео'
        || incomingMessage.request.command?.toLowerCase() === 'геолокация'
        || incomingMessage.request.command?.toLowerCase() === 'разреши геолокацию'
    ) {
        return wrapResponse(
            incomingMessage,
            {
                text: 'Геолокация',
                tts: 'Геолокация',
                directives: {
                    request_geolocation: {}
                },
                end_session: false,
            }
        );
    }

    if (incomingMessage.request.type === 'Geolocation.Allowed') {
        const lat = incomingMessage.session.location?.lat
        const lon = incomingMessage.session.location?.lon
        const accuracy = incomingMessage.session.location?.accuracy

        let text = 'Вы дали доступ к геолокации. Ваша позиция: широта - ' + lat
            + ', долгота - ' + lon + ', точность - ' + accuracy;
        return wrapResponse(incomingMessage, {
                text: text,
                tts: text,
                end_session: false
            }
        );
    }

    if (incomingMessage.request.type === 'Geolocation.Rejected') {

        let text = 'Вы не разрешили доступ к геолокации';
        return wrapResponse(incomingMessage, {
                text: text,
                tts: text,
                end_session: false
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === 'где я'
        || incomingMessage.request.command?.toLowerCase() === 'моя позиция'
        || incomingMessage.request.command?.toLowerCase() === 'локация'
    ) {
        const lat = incomingMessage.session.location?.lat
        const lon = incomingMessage.session.location?.lon
        const accuracy = incomingMessage.session.location?.accuracy

        if (lat && lon && accuracy) {
            let text = 'Ваша позиция: широта - ' + lat + ', долгота - ' + lon + ', точность - ' + accuracy;
            return wrapResponse(incomingMessage, {
                    text: text,
                    tts: text,
                    end_session: false
                }
            );
        } else {
            let text = 'Нет доступа к геолокации. Чтобы разрешить скажи "разреши геолокацию"';
            return wrapResponse(incomingMessage, {
                    text: text,
                    tts: text,
                    end_session: false
                }
            );
        }
    }

    if (incomingMessage.request.command?.toLowerCase() === '65') {
        return wrapResponse(incomingMessage, {
                text: 'Вложенные события апметрики\n',
                end_session: false,
            },
            null,
            null,
            {
                events: [
                    {
                        name: 'custom event'
                    },
                    {
                        name: 'custom event 2',
                        value: {
                            123: 456,
                            789: '10',
                            11: true,
                        },
                    },
                ],
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '66') {
        return wrapResponse(incomingMessage, {
                text: 'Несколько событий апметрики\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event"
                    },
                    {
                        "name": "very custom event",
                        "value": {}
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '67') {
        return wrapResponse(incomingMessage, {
                text: 'Событие апметрики с очень длинным значением\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event"
                    },
                    {
                        "name": "very custom event",
                        "value": {
                            "field": "много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста много текста "
                        }
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '68') {
        return wrapResponse(incomingMessage, {
                text: 'Очень вложенное событие апметрики\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event"
                    },
                    {
                        "name": "very custom event",
                        "value": {
                            "field": {
                                "field": {
                                    "field": {
                                        "field": {
                                            "field": {
                                                "field": {
                                                    "field": {
                                                        "field": {
                                                            "field": {
                                                                "field": {
                                                                    "field": "value"
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '69') {
        return wrapResponse(incomingMessage, {
                text: 'привет я тестовый навык\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event one"
                    },
                    {
                        "name": "custom event one",
                        "value": {
                            "lol": true
                        }
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '70') {
        return wrapResponse(incomingMessage, {
                text: 'События апметрики с одинаковым полем name\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event one",
                        "value": {
                            "123": 456
                        }
                    },
                    {
                        "name": "custom event one",
                        "value": {
                            "123": 456
                        }
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '71') {
        return wrapResponse(incomingMessage, {
                text: 'привет я тестовый навык\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event two",
                        "value": {
                            "123": 789
                        }
                    },
                    {
                        "name": "custom event two",
                        "value": {
                            "123": 456
                        }
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '72') {
        return wrapResponse(incomingMessage, {
                text: 'привет я тестовый навык\n',
                end_session: false,
            },
            null,
            null,
            {
                "events": [
                    {
                        "name": "custom event two",
                        "value": true
                    }
                ]
            }
        );
    }

    if (incomingMessage.request.command?.toLowerCase() === '80') {
        const acceptedUserAgreements = incomingMessage.session?.user.accepted_user_agreements;
        const text = acceptedUserAgreements
            ? 'Пользователь принял все пользовательские соглашения'
            : 'Пользователь не принял пользовательские соглашения';
        return wrapResponse(incomingMessage, {
            text,
            end_session: false,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '81') {
        return wrapResponse(incomingMessage, {
            text: '',
            directives: {
                'show_user_agreements': {}
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '82') {
        const text = 'Прими пользовательское соглашение';
        return wrapResponse(incomingMessage, {
            text,
            directives: {
                'show_user_agreements': {}
            },
            end_session: false,
        });
    }

    if (incomingMessage.request.command?.toLowerCase() === '83') {
        // страшное слово - слово для грепа в логах
        const text = 'Это приватные данные, их не должно быть в логах. клаукверкрякрякря';
        return wrapResponse(incomingMessage, {
            text,
            end_session: false
        }, null, null, {sensitive_data: true});
    }

    return wrapResponse(incomingMessage, {
        text: 'привет я тестовый навык\n',
        end_session: false,
    });
}
