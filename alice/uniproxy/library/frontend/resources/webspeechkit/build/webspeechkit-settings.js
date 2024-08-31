(function (namespace) {
    'use strict';

    if (typeof namespace.ya === 'undefined') {
        namespace.ya = {};
    }
    if (typeof namespace.ya.speechkit === 'undefined') {
        namespace.ya.speechkit = {};
    }
    /**
     * @name settings
     * @static
     * @class Задает глобальные настройки API.
     */

    /**
     * Язык. Возможные значения:
     * <ul>
     *      <li>'ru-RU',</li>
     *      <li>'en-US',</li>
     *      <li>'tr-TR'</li>
     *      <li>'uk-UA' (для синтеза речи).</li>
     * </ul>
     * @field
     * @name settings.lang
     * @type String
     */

    /**
     * <xref scope="external" href="https://tech.yandex.ru/speechkit/jsapi/doc/dg/concepts/settings-docpage/">Языковая модель</xref>.
     * Список доступных значений:
     * <ul>
     *     <li>'notes' (по умолчанию) — общая лексика;</li>
     *     <li>'queries' — короткие запросы;</li>
     *     <li>'names' — имена; </li>
     *     <li>'dates' — даты; </li>
     *     <li>'maps' — топонимы;</li>
     *     <li>'notes' — тексты;</li>
     *     <li>'numbers' — числа.</li>
     * </ul>
     * @field
     * @name settings.model
     * @type String
     */

    /**
     * API-ключ.
     * @field
     * @name settings.apikey
     * @type String
     */

    /**
     * UUID клиента.
     * <p>Формируется автоматически, случайным образом. Является уникальным для каждого пользователя приложения.</p>
     * @field
     * @name settings.uuid
     * @type String
     */

    var settings = {
        websocketProtocol: 'wss://',
        asrUrl: 'webasr.yandex.net/asrsocket.ws',
        ttsUrl: 'https://tts.voicetech.yandex.net',
        ttsStreamUrl: 'webasr.yandex.net/ttssocket.ws',
        lang: 'ru-RU',
        langWhitelist: ['ru-RU', 'tr-TR', 'en-US', 'en-GB', 'uk-UA', 'ru', 'tr', 'en', 'uk', 'en-EN'],
        model: 'notes',
        applicationName: 'jsapi',
        apikey: '',
        uuid: 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(
                /[xy]/g,
                function (c) {
                    var r = Math.random() * 16 | 0;
                    var v = c == 'x' ? r : (r & 0x3 | 0x8);
                    return v.toString(16);
                }
            ),
    };

    if (typeof namespace.ya.speechkit._extend === 'undefined') {
        namespace.ya.speechkit.settings = settings;
    } else {
        namespace.ya.speechkit.settings = namespace.ya.speechkit._extend(settings, namespace.ya.speechkit.settings);
    }
})(this);
